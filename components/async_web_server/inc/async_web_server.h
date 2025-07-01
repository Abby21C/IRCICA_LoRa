#ifndef ASYNC_WEB_SERVER_H
#define ASYNC_WEB_SERVER_H

#include "esp_err.h"
#include "esp_http_server.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Type definition for HTTP request handler function (provided by user)
 */
typedef esp_err_t (*httpd_req_handler_t)(httpd_req_t *req);


/**
 * @brief Initialize the asynchronous Web Server and configure the maximum number of workers
 *
 * @param max_workers Maximum number of concurrent handlers (recommended range: 1~5)
 * @return esp_err_t ESP_OK on success; otherwise returns an ESP error code
 */
esp_err_t async_web_server_init(int max_workers);

/**
 * @brief Submit an asynchronous HTTP request to be executed by the worker pool
 *
 * Note: This function uses `httpd_req_async_handler_begin` to create a copy of the request handle,
 * and is suitable for handlers that take longer to process and need to free up the main thread.
 *
 * @param req Original HTTP request pointer (provided by HTTP Server)
 * @param handler Function that will handle the request (executed in a background thread)
 * @return esp_err_t ESP_OK on success; otherwise returns ESP_FAIL / ESP_ERR_NO_MEM or other errors
 */
esp_err_t async_web_server_submit(httpd_req_t *req, httpd_req_handler_t handler);

#ifdef __cplusplus
}
#endif

#endif // ASYNC_WEB_SERVER_H
