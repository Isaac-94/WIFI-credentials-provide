/*
#####################################################################################################
Desarrollo y testing de software de Sitemas embebidos
IFTS 14
Year: 2024
Author: Isaac Carranza
Title: Wifi connection providing cred with captive portal
Modules: WIFi, NVS, HTTP
#####################################################################################################

*/

#include <stdio.h>
#include <string.h>
#include "esp_event.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_http_server.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "dns_server.h"

#define SSID_KEY "nombrewifi"
#define PASSWORD_KEY "wificontraseña"

#ifndef MIN
#define MIN(x, y) ((x) < (y) ? (x) : (y))
#endif

static const char *TAG = "captive_portal";

// HTML page to capture Wi-Fi credentials
const char html_page[] =
    "<html><body>"
    "<h1>Configuracion WiFi</h1>"
    "<p>Ingrese las credenciales de la red Wifi a la cual desea conectarse /n</p>"
    "<form action=\"/connect\" method=\"POST\">"
    "Nombre de REd WIFI: <input type=\"text\" name=\"ssid\"><br>"
    "Contrasena de WIFI: <input type=\"password\" name=\"password\"><br>"
    "<input type=\"submit\" value=\"Connect\">"
    "</form>"
    "</body></html>";

static char wifi_ssid[32] = {0};
static char wifi_pass[64] = {0};

int retry_num = 0; // intentos para conectar wifi

//--------------------------------------------------------WIFI connect-------------------------
static void wifi_event_handler(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    if (event_id == WIFI_EVENT_STA_START)
    {
        printf("WIFI CONNECTING....\n");
    }
    else if (event_id == WIFI_EVENT_STA_CONNECTED)
    {
        printf("WiFi CONNECTED\n");
    }
    else if (event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        printf("WiFi lost connection\n");
        if (retry_num < 5)
        {
            esp_wifi_connect();
            retry_num++;
            printf("Retrying to Connect...\n");
        }
    }
    else if (event_id == IP_EVENT_STA_GOT_IP)
    {
        printf("Wifi got IP...\n\n");
    }
}

void wifi_connection()
{
    //                          s1.4
    // 2 - Wi-Fi Configuration Phase
    esp_netif_init();
    esp_event_loop_create_default();     // event loop                    s1.2
    esp_netif_create_default_wifi_sta(); // WiFi station                      s1.3
    wifi_init_config_t wifi_initiation = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&wifi_initiation); //
    esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event_handler, NULL);
    esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, wifi_event_handler, NULL);
    wifi_config_t wifi_configuration = {
        .sta = {
            .ssid = "",
            .password = "",

        }

    };
    strcpy((char *)wifi_configuration.sta.ssid, wifi_ssid);
    strcpy((char *)wifi_configuration.sta.password, wifi_pass);
    // esp_log_write(ESP_LOG_INFO, "Kconfig", "SSID=%s, PASS=%s", ssid, pass);
    esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_configuration);
    // 3 - Wi-Fi Start Phase
    esp_wifi_start();
    esp_wifi_set_mode(WIFI_MODE_STA);
    // 4- Wi-Fi Connect Phase
    esp_wifi_connect();
    printf("wifi_init_softap finished. SSID:%s  password:%s", wifi_ssid, wifi_pass);
}
//------------------------------------------------------------ NVS -----------------------------------
void save_wifi_credentials()
{
    nvs_handle_t my_handle;
    esp_err_t err = nvs_open("storage", NVS_READWRITE, &my_handle);
    if (err != ESP_OK)
    {
        printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
        return;
    }
    else
    {
        printf("Abierto correctamente");
    }

    // Guardar SSID
    err = nvs_set_str(my_handle, SSID_KEY, wifi_ssid);
    if (err != ESP_OK)
    {
        printf("Failed to save SSID!\n");
    }
    else
    {
        printf("guardado correctamente");
    }

    // Guardar Contraseña
    err = nvs_set_str(my_handle, PASSWORD_KEY, wifi_pass);
    if (err != ESP_OK)
    {
        printf("Failed to save Password!\n");
    }

    // Confirmar cambios
    err = nvs_commit(my_handle);
    if (err != ESP_OK)
    {
        printf("Failed to commit NVS updates!\n");
    }
    else
    {
        printf("guardado correctamente2");
    }

    nvs_close(my_handle);
}

void load_wifi_credentials()
{
    nvs_handle_t my_handle;
    esp_err_t err = nvs_open("storage", NVS_READONLY, &my_handle);
    if (err != ESP_OK)
    {
        printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
        return;
    }

    // Leer SSID
    size_t ssid_size = sizeof(wifi_ssid);
    err = nvs_get_str(my_handle, SSID_KEY, wifi_ssid, &ssid_size);
    if (err != ESP_OK)
    {
        printf("Failed to read SSID!\n");
    }

    // Leer Contraseña
    size_t pass_size = sizeof(wifi_pass);
    err = nvs_get_str(my_handle, PASSWORD_KEY, wifi_pass, &pass_size);
    if (err != ESP_OK)
    {
        printf("Failed to read Password!\n");
    }

    printf("NVS leída correctamente SSID:%s  password:%s", wifi_ssid, wifi_pass);

    nvs_close(my_handle);
}

//-----------------------------------------------------------CRD PROVIDE-----------------------------
// Handle HTTP GET for the root page (Captive Portal)
static esp_err_t root_get_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, html_page, strlen(html_page));
    return ESP_OK;
}

// Handle HTTP POST for Wi-Fi credentials
static esp_err_t connect_post_handler(httpd_req_t *req)
{
    char buf[100];
    int ret, remaining = req->content_len;

    while (remaining > 0)
    {
        ret = httpd_req_recv(req, buf, MIN(remaining, sizeof(buf)));
        if (ret <= 0)
        {
            return ESP_FAIL;
        }
        remaining -= ret;
        buf[ret] = '\0';

        sscanf(buf, "ssid=%31[^&]&password=%63s", wifi_ssid, wifi_pass); // Parse credentials
    }

    ESP_LOGI(TAG, "Received SSID: %s, Password: %s", wifi_ssid, wifi_pass);

    httpd_resp_set_type(req, "text/html");
    httpd_resp_sendstr(req, "Credentials received. Connecting...");

    wifi_connection();

    save_wifi_credentials();
    /*
        // Now connect to the provided Wi-Fi
        wifi_config_t wifi_config;
        memset(&wifi_config, 0, sizeof(wifi_config_t));
        strcpy((char *)wifi_config.sta.ssid, wifi_ssid);
        strcpy((char *)wifi_config.sta.password, wifi_pass);

        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
        ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
        ESP_ERROR_CHECK(esp_wifi_connect());
    */
    return ESP_OK;
}

static const httpd_uri_t root = {
    .uri = "/",
    .method = HTTP_GET,
    .handler = root_get_handler};

static const httpd_uri_t connect = {
    .uri = "/connect",
    .method = HTTP_POST,
    .handler = connect_post_handler};

static httpd_handle_t start_webserver(void)
{
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    if (httpd_start(&server, &config) == ESP_OK)
    {
        httpd_register_uri_handler(server, &root);
        httpd_register_uri_handler(server, &connect);
    }
    return server;
}

void wifi_init_softap(void)
{
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    wifi_config_t wifi_config = {
        .ap = {
            .ssid = "QMAX_CONFIG_WIFI",
            .ssid_len = strlen("QMAX_CONFIG_WIFI"),
            .password = "12345678",
            .max_connection = 4,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK},
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
}

//------------------------------------------------------------MAIN--------------

void app_main(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    load_wifi_credentials();
    if (strlen(wifi_ssid) == 0)
    {
        wifi_init_softap();

        start_webserver();

        dns_server_config_t dns_config = DNS_SERVER_CONFIG_SINGLE("*", "WIFI_AP_DEF");
        start_dns_server(&dns_config);
    }
    else
    {
        wifi_connection();
    }
}
