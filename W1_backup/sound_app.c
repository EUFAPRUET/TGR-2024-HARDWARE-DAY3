#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <alsa/asoundlib.h>
#include <stdint.h>
#include <curl/curl.h>
#include <time.h>
#include <math.h>
#include <json-c/json.h>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/buffer.h>
#include "db_helper.h"
#include "db_audio_processing.h"
#include <sys/stat.h>

#define THRESHOLD 50
#define FRAMES 4800
#define WAV_HEADER_SIZE 44

// Function to write WAV header
void write_wav_header(FILE *file, int sample_rate, int channels, int num_samples) {
    int byte_rate = sample_rate * channels * sizeof(short);
    int data_size = num_samples * channels * sizeof(short);

    fwrite("RIFF", 1, 4, file);
    int chunk_size = 36 + data_size;
    fwrite(&chunk_size, 4, 1, file);
    fwrite("WAVE", 1, 4, file);
    fwrite("fmt ", 1, 4, file);

    int subchunk1_size = 16;
    short audio_format = 1;
    fwrite(&subchunk1_size, 4, 1, file);
    fwrite(&audio_format, 2, 1, file);
    fwrite(&channels, 2, 1, file);
    fwrite(&sample_rate, 4, 1, file);
    fwrite(&byte_rate, 4, 1, file);
    short block_align = channels * sizeof(short);
    fwrite(&block_align, 2, 1, file);
    short bits_per_sample = 16;
    fwrite(&bits_per_sample, 2, 1, file);

    fwrite("data", 1, 4, file);
    fwrite(&data_size, 4, 1, file);
}

// Function to extract audio features
float extract_feature(short *buffer, int size) {
    float sum = 0;
    for (int i = 0; i < size; i++) {
        sum += abs(buffer[i]);
    }
    return sum / size;
}

// Function to encode audio to Base64
char *encode_audio_to_base64(const char *file_path) {
    FILE *file = fopen(file_path, "rb");
    if (!file) {
        perror("Cannot open audio file");
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    unsigned char *buffer = malloc(file_size);
    fread(buffer, 1, file_size, file);
    fclose(file);

    BIO *bio, *b64;
    BUF_MEM *buffer_ptr;

    b64 = BIO_new(BIO_f_base64());
    bio = BIO_new(BIO_s_mem());
    BIO_push(b64, bio);

    BIO_write(b64, buffer, file_size);
    BIO_flush(b64);
    BIO_get_mem_ptr(b64, &buffer_ptr);

    char *base64_data = malloc(buffer_ptr->length + 1);
    memcpy(base64_data, buffer_ptr->data, buffer_ptr->length);
    base64_data[buffer_ptr->length] = '\0';

    BIO_free_all(b64);
    free(buffer);
    return base64_data;
}

// Function to upload audio and features as JSON
int upload_audio_and_features_json(const char *file_path, const char *url, float feature_value) {
    CURL *curl;
    CURLcode res;
    struct json_object *json_obj = json_object_new_object();

    char *encoded_audio = encode_audio_to_base64(file_path);
    if (!encoded_audio) {
        fprintf(stderr, "Error encoding audio to base64.\n");
        return 1;
    }

    json_object_object_add(json_obj, "audio", json_object_new_string(encoded_audio));
    json_object_object_add(json_obj, "features", json_object_new_double(feature_value));
    const char *json_str = json_object_to_json_string(json_obj);

    const char *api_key = "TNIAPIKey";

    curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();
    if (curl) {
        struct curl_slist *headers = NULL;
        headers = curl_slist_append(headers, "Content-Type: application/json");

        char api_key_header[256];
        snprintf(api_key_header, sizeof(api_key_header), "X-API-KEY: %s", api_key);
        headers = curl_slist_append(headers, api_key_header);

        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_str);

        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        } else {
            printf("File and JSON uploaded successfully!\n");
        }

        curl_easy_cleanup(curl);
        curl_slist_free_all(headers);
    }

    free(encoded_audio);
    json_object_put(json_obj);
    curl_global_cleanup();

    return (int)res;
}

// Main function to record and upload audio
int main(int argc, char *argv[]) {
    int err;
    short buf[FRAMES];
    int flag = 0;
    int sample_count = 0;
    time_t now;
    struct tm *timeinfo;
    snd_pcm_t *capture_handle;
    snd_pcm_hw_params_t *hw_params;
    char file_path[100];
    const char *url = "http://192.168.1.105:5000/api/upload";

    sqlite3 *db;
    sqlite3 *audio_db;
    if (initialize_db(&db) != 0 || initialize_audio_db(&audio_db) != 0) {
        return 1;
    }
    if ((err = snd_pcm_open(&capture_handle, argv[1], SND_PCM_STREAM_CAPTURE, 0)) < 0) {
        fprintf(stderr, "cannot open audio device %s (%s)\n", argv[1], snd_strerror(err));
        return 1;
    }

    snd_pcm_hw_params_alloca(&hw_params);
    unsigned int rate = atoi(argv[2]);
    unsigned int channels = 1;
    snd_pcm_hw_params_any(capture_handle, hw_params);
    snd_pcm_hw_params_set_access(capture_handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED);
    snd_pcm_hw_params_set_format(capture_handle, hw_params, SND_PCM_FORMAT_S16_LE);
    snd_pcm_hw_params_set_rate_near(capture_handle, hw_params, &rate, 0);
    snd_pcm_hw_params_set_channels(capture_handle, hw_params, channels);
    snd_pcm_hw_params(capture_handle, hw_params);

    if ((err = snd_pcm_prepare(capture_handle)) < 0) {
        fprintf(stderr, "unable to prepare audio stream: %s\n", snd_strerror(err));
        return 1;
    }

    printf("Play the sound to record\n");
    FILE *wav_file = NULL;

    while (1) {
        // Read audio frames
        if ((err = snd_pcm_readi(capture_handle, buf, FRAMES)) != FRAMES) {
            fprintf(stderr, "read from audio interface failed (%s)\n", snd_strerror(err));
            return 1;
        }

        float feature = extract_feature(buf, FRAMES);
        time(&now);
        timeinfo = localtime(&now);

        // Start recording only when feature exceeds the threshold
        if (feature > THRESHOLD && !wav_file) {
            // Generate unique file name based on timestamp
            strftime(file_path, sizeof(file_path), "Machine_Sound_%Y%m%d%H%M%S.wav", timeinfo);

            wav_file = fopen(file_path, "wb");
            if (!wav_file) {
                fprintf(stderr, "Cannot open output file.\n");
                return 1;
            }
            write_wav_header(wav_file, rate, channels, 0);
            printf("Recording started: %s\n", file_path);
        }

        // Write audio data to file if recording has started
        if (wav_file && feature > THRESHOLD) {
            fwrite(buf, sizeof(short), FRAMES, wav_file);
            sample_count += FRAMES;
            flag = 1;
        } else if (wav_file && feature < THRESHOLD && flag == 1) {
            // End recording when feature falls below threshold
            fseek(wav_file, 0, SEEK_SET);
            write_wav_header(wav_file, rate, channels, sample_count);
            fclose(wav_file);
            wav_file = NULL;
            printf("Recording ended: %s\n", file_path);

            // Upload the recorded file
            int upload_result = upload_audio_and_features_json(file_path, url, feature);
            

        if (upload_result == 0) {
            printf("Upload completed successfully at %s\n", asctime (timeinfo) );

            // บันทึก timestamp และ data_size ลงในฐานข้อมูล
            struct stat file_stat;
            if (stat(file_path, &file_stat) == 0) {
                float data_size_mb = file_stat.st_size / (1024.0 * 1024.0);  // แปลง byte เป็น MB
                time_t now = time(NULL);
                struct tm *timeinfo = localtime(&now);
                char timestamp[20];
                strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", timeinfo);
                log_data_size(db, timestamp, data_size_mb);
            }
            log_audio_event(audio_db, "upload_to_server_success");
            } else {
            printf("Upload failed with error code: %d\n", upload_result);
            log_audio_event(audio_db, "upload_failed");
            }

            flag = 0;
            sample_count = 0;
        }
    }

    snd_pcm_close(capture_handle);
    close_audio_db(db);
    close_audio_db(audio_db);

    return 0;
}
