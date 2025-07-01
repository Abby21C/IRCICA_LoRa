#include "async_web_server.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "esp_log.h"

#define ASYNC_WORKER_STACK_SIZE 8192

static const char *TAG = "async_web_server";

typedef struct {
    httpd_req_t *req;
    httpd_req_handler_t handler;
} httpd_async_req_t;

static TaskHandle_t *worker_handles = NULL;
static QueueHandle_t async_req_queue = NULL;
static SemaphoreHandle_t worker_ready_count = NULL;
static int max_worker_count = 0;

static void async_req_worker_task(void *p)
{
    ESP_LOGI(TAG, "Async worker started");
    while (true) {
        xSemaphoreGive(worker_ready_count);
        httpd_async_req_t async_req;
        if (xQueueReceive(async_req_queue, &async_req, portMAX_DELAY)) {
            ESP_LOGI(TAG, "Processing async request: %s", async_req.req->uri);
            async_req.handler(async_req.req);
            if (httpd_req_async_handler_complete(async_req.req) != ESP_OK) {
                ESP_LOGE(TAG, "Failed to complete async request");
            }
        }
    }
}

esp_err_t async_web_server_init(int max_workers)
{
    if (max_workers <= 0) return ESP_ERR_INVALID_ARG;

    max_worker_count = max_workers;
    worker_handles = calloc(max_worker_count, sizeof(TaskHandle_t));
    if (!worker_handles) return ESP_ERR_NO_MEM;

    worker_ready_count = xSemaphoreCreateCounting(max_worker_count, 0);
    if (!worker_ready_count) return ESP_ERR_NO_MEM;

    async_req_queue = xQueueCreate(max_worker_count, sizeof(httpd_async_req_t));
    if (!async_req_queue) return ESP_ERR_NO_MEM;

    for (int i = 0; i < max_worker_count; ++i) {
        if (xTaskCreate(async_req_worker_task, "async_worker", ASYNC_WORKER_STACK_SIZE, NULL, 5, &worker_handles[i]) != pdPASS) {
            ESP_LOGE(TAG, "Failed to create worker task %d", i);
            return ESP_FAIL;
        }
    }
    ESP_LOGI(TAG, "Async Web Server initialized with %d workers", max_worker_count);
    return ESP_OK;
}

esp_err_t async_web_server_submit(httpd_req_t *req, httpd_req_handler_t handler)
{
    httpd_req_t *req_copy = NULL;
    esp_err_t err = httpd_req_async_handler_begin(req, &req_copy);
    if (err != ESP_OK) return err;

    httpd_async_req_t async_req = {
        .req = req_copy,
        .handler = handler,
    };

    if (xSemaphoreTake(worker_ready_count, 0) == pdFALSE) {
        ESP_LOGW(TAG, "No worker available");
        httpd_req_async_handler_complete(req_copy);
        return ESP_ERR_NO_MEM;
    }

    if (xQueueSend(async_req_queue, &async_req, pdMS_TO_TICKS(100)) == pdFALSE) {
        ESP_LOGW(TAG, "Async request queue full");
        httpd_req_async_handler_complete(req_copy);
        return ESP_ERR_TIMEOUT;
    }

    return ESP_OK;
}
