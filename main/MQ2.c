#include "esp_adc/adc_oneshot.h"
#include "soc/adc_channel.h"
#include "MQ2.h"

adc_oneshot_unit_handle_t adc_handle;
adc_cali_handle_t cali_handle;

void mq2_init(void)
{
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT_1,
    };
    adc_oneshot_new_unit(&init_config, &adc_handle);

    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN_DB_12,
    };

    adc_oneshot_config_channel(adc_handle, ADC_CHANNEL_6, &config); // GPIO34
}

int mq2_read_raw(void)
{
    int raw = 0;
    adc_oneshot_read(adc_handle, ADC_CHANNEL_6, &raw);
    return raw;
}

int mq2_read_mv(void)
{
    int raw = mq2_read_raw();
    int voltage = 0;
    adc_cali_raw_to_voltage(cali_handle, raw, &voltage);
    return voltage; // in mV
}


