#include <SoapySDR/Device.h>   
#include <SoapySDR/Formats.h>  
#include <stdio.h>             
#include <stdlib.h>            
#include <stdint.h>
#include <complex.h>

int main(){

    SoapySDRKwargs args = {};

    SoapySDRKwargs_set(&args, "driver", "plutosdr");        //тип устройства 
    if (1) {
        SoapySDRKwargs_set(&args, "uri", "usb:");           //Способ обмена сэмплами
    } else {
        SoapySDRKwargs_set(&args, "uri", "ip:192.168.2.1"); 
    }
    SoapySDRKwargs_set(&args, "direct", "1");               
    SoapySDRKwargs_set(&args, "timestamp_every", "1920");   //Размер буфера + временные метки
    SoapySDRKwargs_set(&args, "loopback", "0");             
    SoapySDRDevice *sdr = SoapySDRDevice_make(&args);       
    SoapySDRKwargs_clear(&args);

    int sample_rate = 1e6;
    int carrier_freq = 800e6;

    // Параметры RX части
    SoapySDRDevice_setSampleRate(sdr, SOAPY_SDR_RX, 0, sample_rate);
    SoapySDRDevice_setFrequency(sdr, SOAPY_SDR_RX, 0, carrier_freq , NULL);
    // Параметры TX части
    SoapySDRDevice_setSampleRate(sdr, SOAPY_SDR_TX, 0, sample_rate);
    SoapySDRDevice_setFrequency(sdr, SOAPY_SDR_TX, 0, carrier_freq , NULL);

    // Инициализация количества каналов RX\\TX (один)
    size_t channels[] = {0};
    // Настройки усилителей на RX\\TX
    SoapySDRDevice_setGain(sdr, SOAPY_SDR_RX, 1, 10.0); // Чувствительность приемника
    SoapySDRDevice_setGain(sdr, SOAPY_SDR_TX, 1, -90.0);// Усиление передатчика

    size_t channel_count = sizeof(channels) / sizeof(channels[0]);
    // Формирование потоков для передачи и приема сэмплов
    SoapySDRStream *rxStream = SoapySDRDevice_setupStream(sdr, SOAPY_SDR_RX, SOAPY_SDR_CS16, channels, channel_count, NULL);
    SoapySDRStream *txStream = SoapySDRDevice_setupStream(sdr, SOAPY_SDR_TX, SOAPY_SDR_CS16, channels, channel_count, NULL);

    SoapySDRDevice_activateStream(sdr, rxStream, 0, 0, 0); 
    SoapySDRDevice_activateStream(sdr, txStream, 0, 0, 0); 
    // Получение размера буферов
    size_t rx_mtu = SoapySDRDevice_getStreamMTU(sdr, rxStream);
    size_t tx_mtu = SoapySDRDevice_getStreamMTU(sdr, txStream);

    // Выделяем память под буферы RX TX
    int16_t tx_buff[2*tx_mtu];
    int16_t rx_buffer[2*rx_mtu];

        //Прямоугольный сигнал
        for (int i = 0; i < 2 * tx_mtu; i+=2)
        {
            double t = (double)(i / 2) / tx_mtu * 2.0 - 1.0;
            double rect_value = (fabs(t) < 0.8) ? 1.0 : 0.0;
            tx_buff[i] = (int16_t)(rect_value * 16000);   // I — прямоугольник
            tx_buff[i + 1] = 0; // Q = 0
        }

        for(size_t i = 0; i < 2; i++)
        {
            tx_buff[0 + i] = 0xffff;
            // 8 x timestamp words
            tx_buff[10 + i] = 0xffff;
        }
        const long  timeoutUs = 400000;
    long long last_time = 0;
    // Количество итерация чтения из буфера
    size_t iteration_count = 10;

    FILE *file = fopen("txdata.pcm", "w");

    //получение и отправка сэмплов
    for (size_t buffers_read = 0; buffers_read < iteration_count; buffers_read++)
    {
        void *rx_buffs[] = {rx_buffer};
        int flags;        
        long long timeNs; 

        // считали буффер RX, записали его в rx_buffer
        int sr = SoapySDRDevice_readStream(sdr, rxStream, rx_buffs, rx_mtu, &flags, &timeNs, timeoutUs);
        fwrite(rx_buffer, 2* rx_mtu * sizeof(int16_t), 1, file);
        // Смотрим на количество считаных сэмплов, времени прихода и разницы во времени с чтением прошлого буфера
        printf("Buffer: %lu - Samples: %i, Flags: %i, Time: %lli, TimeDiff: %lli\n", buffers_read, sr, flags, timeNs, timeNs - last_time);
        last_time = timeNs;

        // Переменная для времени отправки сэмплов относительно текущего приема
        long long tx_time = timeNs + (4 * 1000 * 1000); // на 4мс в будущее

        // Добавляем время, когда нужно передать блок tx_buff, через tx_time -наносекунд
        for(size_t i = 0; i < 8; i++)
        {
            uint8_t tx_time_byte = (tx_time >> (i * 8)) & 0xff;
            tx_buff[2 + i] = tx_time_byte << 4;
        }

        //отправляем tx_buff массив
        void *tx_buffs[] = {tx_buff};
        if( (buffers_read == 2) ){
            printf("buffers_read: %d\n", buffers_read);
            flags = SOAPY_SDR_HAS_TIME;
            int st = SoapySDRDevice_writeStream(sdr, txStream, (const void * const*)tx_buffs, tx_mtu, &flags, tx_time, timeoutUs);
            if ((size_t)st != tx_mtu)
            {
                printf("TX Failed: %i\n", st);
            }
        }
    }
    fclose(file);

    SoapySDRDevice_deactivateStream(sdr, rxStream, 0, 0);
    SoapySDRDevice_deactivateStream(sdr, txStream, 0, 0);

    SoapySDRDevice_closeStream(sdr, rxStream);
    SoapySDRDevice_closeStream(sdr, txStream);

    SoapySDRDevice_unmake(sdr);

    return 0;
    }