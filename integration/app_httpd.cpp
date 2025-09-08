#include "esp_http_server.h"
#include "esp_timer.h"
#include "esp_camera.h"
#include "img_converters.h"
#include "Arduino.h"
#include "camera_index.h"
#include "FS.h"
#include "SD_MMC.h"

typedef struct {
        httpd_req_t *req;
        size_t len;
} jpg_chunking_t;

#define PART_BOUNDARY "123456789000000000000987654321"
static const char* _STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
static const char* _STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
static const char* _STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

httpd_handle_t stream_httpd = NULL;
httpd_handle_t camera_httpd = NULL;

static size_t jpg_encode_stream(void * arg, size_t index, const void* data, size_t len){
    jpg_chunking_t *j = (jpg_chunking_t *)arg;
    if(!index){
        j->len = 0;
    }
    if(httpd_resp_send_chunk(j->req, (const char *)data, len) != ESP_OK){
        return 0;
    }
    j->len += len;
    return len;
}

static esp_err_t capture_handler(httpd_req_t *req){
    Serial.println("DEBUG: /capture endpoint called");
    camera_fb_t * fb = NULL;
    esp_err_t res = ESP_OK;
    int64_t fr_start = esp_timer_get_time();

    fb = esp_camera_fb_get();
    if (!fb) {
        Serial.println("ERROR: Camera capture failed in /capture handler");
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }
    Serial.printf("DEBUG: Captured frame: %dx%d, %u bytes, format: %d\n", 
                  fb->width, fb->height, fb->len, fb->format);

    httpd_resp_set_type(req, "image/jpeg");
    httpd_resp_set_hdr(req, "Content-Disposition", "inline; filename=capture.jpg");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");

    size_t fb_len = 0;
    if(fb->format == PIXFORMAT_JPEG){
        fb_len = fb->len;
        res = httpd_resp_send(req, (const char *)fb->buf, fb->len);
        Serial.printf("DEBUG: Sent JPEG directly, %u bytes\n", fb_len);
    } else {
        jpg_chunking_t jchunk = { req, 0 };
        res = frame2jpg_cb(fb, 80, jpg_encode_stream, &jchunk)?ESP_OK:ESP_FAIL;
        httpd_resp_send_chunk(req, NULL, 0);
        fb_len = jchunk.len;
        Serial.printf("DEBUG: Converted and sent frame, %u bytes\n", fb_len);
    }
    esp_camera_fb_return(fb);
    int64_t fr_end = esp_timer_get_time();
    Serial.printf("DEBUG: /capture completed in %ums, sent %uB\n", 
                  (uint32_t)((fr_end - fr_start)/1000), (uint32_t)(fb_len));
    return res;
}

static esp_err_t stream_handler(httpd_req_t *req){
    Serial.println("DEBUG: /stream endpoint called");
    camera_fb_t * fb = NULL;
    esp_err_t res = ESP_OK;
    size_t _jpg_buf_len;
    uint8_t * _jpg_buf;
    char * part_buf[64];
    static int64_t last_frame = 0;
    if(!last_frame) {
        last_frame = esp_timer_get_time();
    }

    res = httpd_resp_set_type(req, _STREAM_CONTENT_TYPE);
    if(res != ESP_OK){
        Serial.printf("ERROR: Failed to set stream content type: %d\n", res);
        return res;
    }

    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    Serial.println("DEBUG: Stream headers set, starting frame loop");

    int frame_count = 0;
    while(true){
        fb = esp_camera_fb_get();
        if (!fb) {
            Serial.printf("ERROR: Camera capture failed on frame %d\n", frame_count);
            res = ESP_FAIL;
            break;
        }
        
        if(fb->format != PIXFORMAT_JPEG){
            bool jpeg_converted = frame2jpg(fb, 80, &_jpg_buf, &_jpg_buf_len);
            if(!jpeg_converted){
                Serial.printf("ERROR: JPEG compression failed on frame %d\n", frame_count);
                esp_camera_fb_return(fb);
                res = ESP_FAIL;
                break;
            }
        } else {
            _jpg_buf_len = fb->len;
            _jpg_buf = fb->buf;
        }

        if(res == ESP_OK){
            res = httpd_resp_send_chunk(req, _STREAM_BOUNDARY, strlen(_STREAM_BOUNDARY));
        }
        if(res == ESP_OK){
            size_t hlen = snprintf((char *)part_buf, 64, _STREAM_PART, _jpg_buf_len);
            res = httpd_resp_send_chunk(req, (const char *)part_buf, hlen);
        }
        if(res == ESP_OK){
            res = httpd_resp_send_chunk(req, (const char *)_jpg_buf, _jpg_buf_len);
        }
        if(fb->format != PIXFORMAT_JPEG){
            free(_jpg_buf);
        }
        esp_camera_fb_return(fb);
        if(res != ESP_OK){
            Serial.printf("ERROR: Stream failed at frame %d with error %d\n", frame_count, res);
            break;
        }
        int64_t fr_end = esp_timer_get_time();
        int64_t frame_time = fr_end - last_frame;
        last_frame = fr_end;
        frame_time /= 1000;
        
        frame_count++;
        if(frame_count % 30 == 0) { // Log every 30th frame to avoid spam
            Serial.printf("DEBUG: Stream frame %d: %uB %ums (%.1ffps)\n",
                frame_count, (uint32_t)(_jpg_buf_len), (uint32_t)frame_time, 1000.0 / (uint32_t)frame_time);
        }
    }

    last_frame = 0;
    Serial.printf("DEBUG: Stream ended after %d frames\n", frame_count);
    return res;
}

static esp_err_t cmd_handler(httpd_req_t *req){
    Serial.println("DEBUG: /control endpoint called");
    char*  buf;
    size_t buf_len;
    char variable[32] = {0,};
    char value[32] = {0,};

    buf_len = httpd_req_get_url_query_len(req) + 1;
    if (buf_len > 1) {
        buf = (char*)malloc(buf_len);
        if(!buf){
            Serial.println("ERROR: Failed to allocate memory for control query");
            httpd_resp_send_500(req);
            return ESP_FAIL;
        }
        if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
            if (httpd_query_key_value(buf, "var", variable, sizeof(variable)) == ESP_OK &&
                httpd_query_key_value(buf, "val", value, sizeof(value)) == ESP_OK) {
                Serial.printf("DEBUG: Control command - var: %s, val: %s\n", variable, value);
            } else {
                Serial.println("ERROR: Failed to parse control query parameters");
                free(buf);
                httpd_resp_send_404(req);
                return ESP_FAIL;
            }
        } else {
            Serial.println("ERROR: Failed to get control query string");
            free(buf);
            httpd_resp_send_404(req);
            return ESP_FAIL;
        }
        free(buf);
    } else {
        Serial.println("ERROR: No query parameters in control request");
        httpd_resp_send_404(req);
        return ESP_FAIL;
    }

    int val = atoi(value);
    sensor_t * s = esp_camera_sensor_get();
    int res = 0;

    if(!strcmp(variable, "framesize")) res = s->set_framesize(s, (framesize_t)val);
    else if(!strcmp(variable, "quality")) res = s->set_quality(s, val);
    else if(!strcmp(variable, "contrast")) res = s->set_contrast(s, val);
    else if(!strcmp(variable, "brightness")) res = s->set_brightness(s, val);
    else if(!strcmp(variable, "saturation")) res = s->set_saturation(s, val);
    else if(!strcmp(variable, "gainceiling")) res = s->set_gainceiling(s, (gainceiling_t)val);
    else if(!strcmp(variable, "colorbar")) res = s->set_colorbar(s, val);
    else if(!strcmp(variable, "awb")) res = s->set_whitebal(s, val);
    else if(!strcmp(variable, "agc")) res = s->set_gain_ctrl(s, val);
    else if(!strcmp(variable, "aec")) res = s->set_exposure_ctrl(s, val);
    else if(!strcmp(variable, "hmirror")) res = s->set_hmirror(s, val);
    else if(!strcmp(variable, "vflip")) res = s->set_vflip(s, val);
    else if(!strcmp(variable, "awb_gain")) res = s->set_awb_gain(s, val);
    else if(!strcmp(variable, "agc_gain")) res = s->set_agc_gain(s, val);
    else if(!strcmp(variable, "aec_value")) res = s->set_aec_value(s, val);
    else if(!strcmp(variable, "aec2")) res = s->set_aec2(s, val);
    else if(!strcmp(variable, "dcw")) res = s->set_dcw(s, val);
    else if(!strcmp(variable, "bpc")) res = s->set_bpc(s, val);
    else if(!strcmp(variable, "wpc")) res = s->set_wpc(s, val);
    else if(!strcmp(variable, "raw_gma")) res = s->set_raw_gma(s, val);
    else if(!strcmp(variable, "lenc")) res = s->set_lenc(s, val);
    else if(!strcmp(variable, "special_effect")) res = s->set_special_effect(s, val);
    else if(!strcmp(variable, "wb_mode")) res = s->set_wb_mode(s, val);
    else if(!strcmp(variable, "ae_level")) res = s->set_ae_level(s, val);
    else {
        Serial.printf("ERROR: Unknown control variable: %s\n", variable);
        res = -1;
    }

    if(res){
        Serial.printf("ERROR: Camera control failed for %s=%s (result: %d)\n", variable, value, res);
        return httpd_resp_send_500(req);
    }

    Serial.printf("DEBUG: Camera control successful: %s=%s\n", variable, value);
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    return httpd_resp_send(req, NULL, 0);
}

static esp_err_t status_handler(httpd_req_t *req){
    Serial.println("DEBUG: /status endpoint called");
    static char json_response[1024];

    sensor_t * s = esp_camera_sensor_get();
    char * p = json_response;
    *p++ = '{';

    p+=sprintf(p, "\"framesize\":%u,", s->status.framesize);
    p+=sprintf(p, "\"quality\":%u,", s->status.quality);
    p+=sprintf(p, "\"brightness\":%d,", s->status.brightness);
    p+=sprintf(p, "\"contrast\":%d,", s->status.contrast);
    p+=sprintf(p, "\"saturation\":%d,", s->status.saturation);
    p+=sprintf(p, "\"special_effect\":%u,", s->status.special_effect);
    p+=sprintf(p, "\"wb_mode\":%u,", s->status.wb_mode);
    p+=sprintf(p, "\"awb\":%u,", s->status.awb);
    p+=sprintf(p, "\"awb_gain\":%u,", s->status.awb_gain);
    p+=sprintf(p, "\"aec\":%u,", s->status.aec);
    p+=sprintf(p, "\"aec2\":%u,", s->status.aec2);
    p+=sprintf(p, "\"ae_level\":%d,", s->status.ae_level);
    p+=sprintf(p, "\"aec_value\":%u,", s->status.aec_value);
    p+=sprintf(p, "\"agc\":%u,", s->status.agc);
    p+=sprintf(p, "\"agc_gain\":%u,", s->status.agc_gain);
    p+=sprintf(p, "\"gainceiling\":%u,", s->status.gainceiling);
    p+=sprintf(p, "\"bpc\":%u,", s->status.bpc);
    p+=sprintf(p, "\"wpc\":%u,", s->status.wpc);
    p+=sprintf(p, "\"raw_gma\":%u,", s->status.raw_gma);
    p+=sprintf(p, "\"lenc\":%u,", s->status.lenc);
    p+=sprintf(p, "\"hmirror\":%u,", s->status.hmirror);
    p+=sprintf(p, "\"vflip\":%u,", s->status.vflip);
    p+=sprintf(p, "\"dcw\":%u,", s->status.dcw);
    p+=sprintf(p, "\"colorbar\":%u", s->status.colorbar);
    *p++ = '}';
    *p++ = 0;
    
    Serial.printf("DEBUG: Status JSON generated (%d bytes)\n", strlen(json_response));
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    return httpd_resp_send(req, json_response, strlen(json_response));
}

static esp_err_t index_handler(httpd_req_t *req){
    Serial.println("DEBUG: / (index) endpoint called");
    httpd_resp_set_type(req, "text/html");
    Serial.printf("DEBUG: Sending HTML page (%d bytes)\n", strlen(INDEX_HTML));
    return httpd_resp_send(req, (const char *)INDEX_HTML, strlen(INDEX_HTML));
}

static esp_err_t save_photo_handler(httpd_req_t *req)
{
    Serial.println("DEBUG: /save-photo endpoint called");
    camera_fb_t * fb = NULL;
    fb = esp_camera_fb_get();
    if (!fb) {
        Serial.println("ERROR: Camera capture failed in save-photo handler");
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }
    Serial.printf("DEBUG: Captured photo for saving: %dx%d, %u bytes, format: %d\n", 
                  fb->width, fb->height, fb->len, fb->format);

    // Find next available filename
    int photo_index = 0;
    while(SD_MMC.exists("/camera/" + String(photo_index) +".jpg")) {
        photo_index++;
    }
    String path = "/camera/" + String(photo_index) +".jpg";
    Serial.printf("DEBUG: Saving photo as %s\n", path.c_str());
    
    fs::FS &fs = SD_MMC;
    File file = fs.open(path.c_str(), FILE_WRITE);
    if(file){
        size_t written = file.write(fb->buf, fb->len);
        file.close();
        Serial.printf("INFO: Photo saved successfully to %s (%u bytes written)\n", path.c_str(), written);
        
        // Verify the file was written
        if(SD_MMC.exists(path.c_str())) {
            File checkFile = SD_MMC.open(path.c_str(), FILE_READ);
            Serial.printf("DEBUG: Verification - file size on disk: %u bytes\n", checkFile.size());
            checkFile.close();
        }
    } else {
        Serial.printf("ERROR: Failed to open %s for writing\n", path.c_str());
        esp_camera_fb_return(fb);
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }
    
    esp_camera_fb_return(fb);
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_send(req, "Photo saved", HTTPD_RESP_USE_STRLEN);
    Serial.println("DEBUG: /save-photo response sent");
    return ESP_OK;
}

String listFilesJSON(fs::FS &fs, const char * dirname){
    Serial.printf("DEBUG: Listing files in directory: %s\n", dirname);
    String json = "[";
    File root = fs.open(dirname);
    if(!root){
        Serial.printf("ERROR: Failed to open directory %s for listing\n", dirname);
        return "[]";
    }
    if(!root.isDirectory()){
        Serial.printf("ERROR: %s is not a directory\n", dirname);
        return "[]";
    }

    int fileCount = 0;
    File file = root.openNextFile();
    while(file){
        if(!file.isDirectory()) { 
            if(json != "["){
                json += ",";
            }
            String filename = String(file.name());
            int lastSlash = filename.lastIndexOf('/');
            if (lastSlash != -1) {
                filename = filename.substring(lastSlash + 1);
            }
            json += "\"" + filename + "\"";
            fileCount++;
            Serial.printf("DEBUG: Found file: %s (%u bytes)\n", filename.c_str(), file.size());
        }
        file.close();
        file = root.openNextFile();
    }
    root.close();
    json += "]";
    Serial.printf("DEBUG: Directory listing complete: %d files found\n", fileCount);
    Serial.printf("DEBUG: JSON response: %s\n", json.c_str());
    return json;
}

static esp_err_t photos_handler(httpd_req_t *req){
    Serial.println("DEBUG: /photos endpoint called");
    String json = listFilesJSON(SD_MMC, "/camera");
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    Serial.printf("DEBUG: Sending photos list (%d bytes)\n", json.length());
    httpd_resp_send(req, json.c_str(), json.length());
    return ESP_OK;
}

static esp_err_t image_handler(httpd_req_t *req){
    String filename = &req->uri[7]; // remove "/image/"
    String fullPath = "/camera/" + filename;
    Serial.printf("DEBUG: /image endpoint called for: %s\n", filename.c_str());
    Serial.printf("DEBUG: Full path: %s\n", fullPath.c_str());
    
    // Check if file exists
    if (!SD_MMC.exists(fullPath)) {
        Serial.printf("ERROR: File does not exist: %s\n", fullPath.c_str());
        
        // List available files for debugging
        Serial.println("DEBUG: Available files in /camera/:");
        File root = SD_MMC.open("/camera");
        if(root && root.isDirectory()) {
            File file = root.openNextFile();
            int count = 0;
            while(file) {
                if(!file.isDirectory()) {
                    Serial.printf("  - %s (%u bytes)\n", file.name(), file.size());
                    count++;
                }
                file.close();
                file = root.openNextFile();
            }
            root.close();
            Serial.printf("DEBUG: Total %d files found\n", count);
        }
        
        httpd_resp_send_404(req);
        return ESP_FAIL;
    }

    File file = SD_MMC.open(fullPath, FILE_READ);
    if(!file){
        Serial.printf("ERROR: Failed to open %s for reading\n", fullPath.c_str());
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }
    
    size_t fileSize = file.size();
    Serial.printf("DEBUG: Serving image %s (%u bytes)\n", filename.c_str(), fileSize);
    
    httpd_resp_set_type(req, "image/jpeg");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    
    uint8_t *buf = (uint8_t *)malloc(fileSize);
    if (!buf) {
        Serial.printf("ERROR: Failed to allocate %u bytes for image buffer\n", fileSize);
        file.close();
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }
    
    size_t bytesRead = file.read(buf, fileSize);
    file.close();
    Serial.printf("DEBUG: Read %u bytes from file\n", bytesRead);
    
    esp_err_t result = httpd_resp_send(req, (const char *)buf, fileSize);
    free(buf);
    
    if(result == ESP_OK) {
        Serial.printf("DEBUG: Image %s served successfully\n", filename.c_str());
    } else {
        Serial.printf("ERROR: Failed to send image %s (error: %d)\n", filename.c_str(), result);
    }
    
    return result;
}

void startCameraServer(){
    Serial.println("DEBUG: Starting camera web server");
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = 80;

    httpd_uri_t index_uri = {
        .uri       = "/",
        .method    = HTTP_GET,
        .handler   = index_handler,
        .user_ctx  = NULL
    };

    httpd_uri_t status_uri = {
        .uri       = "/status",
        .method    = HTTP_GET,
        .handler   = status_handler,
        .user_ctx  = NULL
    };

    httpd_uri_t cmd_uri = {
        .uri       = "/control",
        .method    = HTTP_GET,
        .handler   = cmd_handler,
        .user_ctx  = NULL
    };

    httpd_uri_t capture_uri = {
        .uri       = "/capture",
        .method    = HTTP_GET,
        .handler   = capture_handler,
        .user_ctx  = NULL
    };

    httpd_uri_t stream_uri = {
        .uri       = "/stream",
        .method    = HTTP_GET,
        .handler   = stream_handler,
        .user_ctx  = NULL
    };
    
    httpd_uri_t save_photo_uri = {
        .uri       = "/save-photo",
        .method    = HTTP_GET,
        .handler   = save_photo_handler,
        .user_ctx  = NULL
    };

    httpd_uri_t photos_uri = {
        .uri       = "/photos",
        .method    = HTTP_GET,
        .handler   = photos_handler,
        .user_ctx  = NULL
    };

    httpd_uri_t image_uri = {
        .uri       = "/image/*",
        .method    = HTTP_GET,
        .handler   = image_handler,
        .user_ctx  = NULL
    };

    Serial.printf("DEBUG: Starting main web server on port: %d\n", config.server_port);
    if (httpd_start(&camera_httpd, &config) == ESP_OK) {
        Serial.println("DEBUG: Registering URI handlers...");
        httpd_register_uri_handler(camera_httpd, &index_uri);
        httpd_register_uri_handler(camera_httpd, &cmd_uri);
        httpd_register_uri_handler(camera_httpd, &status_uri);
        httpd_register_uri_handler(camera_httpd, &capture_uri);
        httpd_register_uri_handler(camera_httpd, &save_photo_uri);
        httpd_register_uri_handler(camera_httpd, &photos_uri);
        httpd_register_uri_handler(camera_httpd, &image_uri);
        Serial.println("DEBUG: Main server started successfully");
    } else {
        Serial.println("ERROR: Failed to start main web server");
    }

    config.server_port += 1;
    config.ctrl_port += 1;
    Serial.printf("DEBUG: Starting stream server on port: %d\n", config.server_port);
    if (httpd_start(&stream_httpd, &config) == ESP_OK) {
        httpd_register_uri_handler(stream_httpd, &stream_uri);
        Serial.println("DEBUG: Stream server started successfully");
    } else {
        Serial.println("ERROR: Failed to start stream server");
    }
    
    Serial.println("DEBUG: Camera web server initialization complete");
}