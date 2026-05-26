#include "ble_gatt.h"
#include "data_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include <string.h>

#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "host/ble_uuid.h"
#include "host/util/util.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"

static const char *TAG = "BLE";
#define DEVICE_NAME "Biorreactor"

static uint16_t s_conn_handle       = 0;
static bool     s_connected         = false;
static bool     s_notify_enabled    = false;
static bool     s_hist_notify_en    = false;
static bool     s_ble_ready         = false;
static uint16_t s_sensor_val_handle = 0;
static uint16_t s_hist_val_handle   = 0;
static uint8_t  s_own_addr_type     = 0;

static ble_sensor_packet_t s_sensor_data;
static ble_cmd_callback_t  s_cmd_cb = NULL;

static const ble_uuid128_t svc_uuid = BLE_UUID128_INIT(
    0xBC, 0x9A, 0x78, 0x56, 0x34, 0x12, 0xF0, 0xDE,
    0xBC, 0x9A, 0x78, 0x56, 0x01, 0x00, 0x34, 0x12);

static const ble_uuid128_t sensor_uuid = BLE_UUID128_INIT(
    0xBC, 0x9A, 0x78, 0x56, 0x34, 0x12, 0xF0, 0xDE,
    0xBC, 0x9A, 0x78, 0x56, 0x02, 0x00, 0x34, 0x12);

static const ble_uuid128_t ctrl_uuid = BLE_UUID128_INIT(
    0xBC, 0x9A, 0x78, 0x56, 0x34, 0x12, 0xF0, 0xDE,
    0xBC, 0x9A, 0x78, 0x56, 0x03, 0x00, 0x34, 0x12);

static const ble_uuid128_t hist_uuid = BLE_UUID128_INIT(
    0xBC, 0x9A, 0x78, 0x56, 0x34, 0x12, 0xF0, 0xDE,
    0xBC, 0x9A, 0x78, 0x56, 0x04, 0x00, 0x34, 0x12);

static int  ble_gap_event_handler(struct ble_gap_event *event, void *arg);
static void ble_advertise(void);

static int sensor_chr_access(uint16_t conn_handle, uint16_t attr_handle,
                              struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    if (ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR) {
        os_mbuf_append(ctxt->om, &s_sensor_data, sizeof(s_sensor_data));
        return 0;
    }
    return BLE_ATT_ERR_UNLIKELY;
}

static int hist_chr_access(uint16_t conn_handle, uint16_t attr_handle,
                            struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    if (ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR) {
        uint16_t count = (uint16_t)data_log_get_count();
        os_mbuf_append(ctxt->om, &count, sizeof(count));
        return 0;
    }
    return BLE_ATT_ERR_UNLIKELY;
}

static void hist_download_task(void *param)
{
    int count = data_log_get_count();
    ESP_LOGI(TAG, "Descargando historial: %d entradas...", count);

    for (int i = 0; i < count; i++) {
        if (!s_connected) {
            ESP_LOGW(TAG, "Descarga abortada: desconexion");
            break;
        }

        uint8_t buf[10];
        uint16_t idx = (uint16_t)i;
        uint32_t t   = data_log_get_time(i);
        float temp   = data_log_get_temp(i);

        memcpy(&buf[0], &idx,  2);
        memcpy(&buf[2], &t,    4);
        memcpy(&buf[6], &temp, 4);

        struct os_mbuf *om = ble_hs_mbuf_from_flat(buf, sizeof(buf));
        if (om) {
            ble_gatts_notify_custom(s_conn_handle, s_hist_val_handle, om);
        }
        vTaskDelay(pdMS_TO_TICKS(20));
    }

    if (s_connected) {
        uint8_t end[4];
        uint16_t marker = 0xFFFF;
        uint16_t cnt    = (uint16_t)count;
        memcpy(&end[0], &marker, 2);
        memcpy(&end[2], &cnt,    2);

        struct os_mbuf *om = ble_hs_mbuf_from_flat(end, sizeof(end));
        if (om) {
            ble_gatts_notify_custom(s_conn_handle, s_hist_val_handle, om);
        }
        ESP_LOGI(TAG, "Descarga completa: %d entradas enviadas", count);
    }

    vTaskDelete(NULL);
}

static int ctrl_chr_access(uint16_t conn_handle, uint16_t attr_handle,
                            struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    if (ctxt->op == BLE_GATT_ACCESS_OP_WRITE_CHR) {
        uint16_t len = OS_MBUF_PKTLEN(ctxt->om);

        if (len == 5 && s_cmd_cb != NULL) {
            uint8_t buf[5];
            os_mbuf_copydata(ctxt->om, 0, 5, buf);

            uint8_t cmd = buf[0];
            float val;
            memcpy(&val, &buf[1], sizeof(float));

            s_cmd_cb(cmd, val);
            ESP_LOGI(TAG, "CMD PID: tipo=%d valor=%.2f", cmd, val);
        }
        else if (len == 1) {
            uint8_t cmd;
            os_mbuf_copydata(ctxt->om, 0, 1, &cmd);

            if (cmd == BLE_CMD_HIST_DOWNLOAD) {
                if (s_hist_notify_en) {
                    ESP_LOGI(TAG, "Iniciando descarga de historial...");
                    xTaskCreate(hist_download_task, "hist_dl", 4096, NULL, 5, NULL);
                } else {
                    ESP_LOGW(TAG, "Descarga: suscribete primero a notificaciones");
                }
            }
            else if (cmd == BLE_CMD_HIST_RESET) {
                data_log_reset();
                ESP_LOGI(TAG, "Historial reseteado por BLE");
            }
        }
        return 0;
    }
    return BLE_ATT_ERR_UNLIKELY;
}

static const struct ble_gatt_svc_def gatt_svcs[] = {
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = &svc_uuid.u,
        .characteristics = (struct ble_gatt_chr_def[]) {
            {
                .uuid       = &sensor_uuid.u,
                .access_cb  = sensor_chr_access,
                .val_handle = &s_sensor_val_handle,
                .flags      = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY,
            },
            {
                .uuid       = &ctrl_uuid.u,
                .access_cb  = ctrl_chr_access,
                .flags      = BLE_GATT_CHR_F_WRITE,
            },
            {
                .uuid       = &hist_uuid.u,
                .access_cb  = hist_chr_access,
                .val_handle = &s_hist_val_handle,
                .flags      = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY,
            },
            { 0 },
        },
    },
    { 0 },
};

static int ble_gap_event_handler(struct ble_gap_event *event, void *arg)
{
    switch (event->type) {

        case BLE_GAP_EVENT_CONNECT:
            if (event->connect.status == 0) {
                s_conn_handle = event->connect.conn_handle;
                s_connected   = true;
                ESP_LOGI(TAG, "Cliente BLE conectado (entries=%d)",
                         data_log_get_count());
            } else {
                s_connected = false;
                ble_advertise();
            }
            break;

        case BLE_GAP_EVENT_DISCONNECT:
            s_connected      = false;
            s_notify_enabled = false;
            s_hist_notify_en = false;
            ESP_LOGW(TAG, "Cliente BLE desconectado");
            ble_advertise();
            break;

        case BLE_GAP_EVENT_SUBSCRIBE:
            if (event->subscribe.attr_handle == s_sensor_val_handle) {
                s_notify_enabled = event->subscribe.cur_notify;
                ESP_LOGI(TAG, "Sensor notify %s",
                         s_notify_enabled ? "ON" : "OFF");
            }
            if (event->subscribe.attr_handle == s_hist_val_handle) {
                s_hist_notify_en = event->subscribe.cur_notify;
                ESP_LOGI(TAG, "Historial notify %s",
                         s_hist_notify_en ? "ON" : "OFF");
            }
            break;

        case BLE_GAP_EVENT_ADV_COMPLETE:
            ble_advertise();
            break;

        case BLE_GAP_EVENT_MTU:
            ESP_LOGI(TAG, "MTU: %d", event->mtu.value);
            break;
    }
    return 0;
}

static void ble_advertise(void)
{
    struct ble_hs_adv_fields fields;
    memset(&fields, 0, sizeof(fields));

    fields.flags                 = BLE_HS_ADV_F_DISC_GEN |
                                   BLE_HS_ADV_F_BREDR_UNSUP;
    fields.tx_pwr_lvl_is_present = 1;
    fields.tx_pwr_lvl            = BLE_HS_ADV_TX_PWR_LVL_AUTO;
    fields.name                  = (uint8_t *)DEVICE_NAME;
    fields.name_len              = strlen(DEVICE_NAME);
    fields.name_is_complete      = 1;

    ble_gap_adv_set_fields(&fields);

    struct ble_gap_adv_params adv_params;
    memset(&adv_params, 0, sizeof(adv_params));
    adv_params.conn_mode = BLE_GAP_CONN_MODE_UND;
    adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;

    ble_gap_adv_start(s_own_addr_type, NULL, BLE_HS_FOREVER,
                      &adv_params, ble_gap_event_handler, NULL);
    ESP_LOGI(TAG, "Advertising...");
}

static void ble_on_sync(void)
{
    ble_hs_id_infer_auto(0, &s_own_addr_type);
    ble_advertise();
    s_ble_ready = true;
    ESP_LOGI(TAG, "BLE listo — '%s'", DEVICE_NAME);
}

static void ble_on_reset(int reason)
{
    ESP_LOGE(TAG, "BLE host reset, reason=%d", reason);
}

static void ble_host_task(void *param)
{
    nimble_port_run();
    nimble_port_freertos_deinit();
}

void ble_gatt_init(ble_cmd_callback_t cmd_cb)
{
    s_cmd_cb = cmd_cb;

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
        ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_ERROR_CHECK(nimble_port_init());

    ble_hs_cfg.sync_cb  = ble_on_sync;
    ble_hs_cfg.reset_cb = ble_on_reset;

    ble_svc_gap_init();
    ble_svc_gatt_init();
    ble_gatts_count_cfg(gatt_svcs);
    ble_gatts_add_svcs(gatt_svcs);
    ble_svc_gap_device_name_set(DEVICE_NAME);

    nimble_port_freertos_init(ble_host_task);
    ESP_LOGI(TAG, "BLE GATT Server inicializado");
}

void ble_gatt_update_data(const ble_sensor_packet_t *data)
{
    if (!s_ble_ready) return;

    memcpy(&s_sensor_data, data, sizeof(ble_sensor_packet_t));

    if (s_connected && s_notify_enabled) {
        struct os_mbuf *om = ble_hs_mbuf_from_flat(
            &s_sensor_data, sizeof(s_sensor_data));
        if (om) {
            ble_gatts_notify_custom(s_conn_handle,
                                    s_sensor_val_handle, om);
        }
    }
}

bool ble_gatt_is_connected(void)
{
    return s_connected;
}