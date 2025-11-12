#include <SoapySDR/Device.h>   
#include <SoapySDR/Formats.h>  
#include <stdlib.h>            
#include <stdint.h>
#include <complex.h>
#include <stdio.h>
#include <math.h>

void to_bpsk(int bits[], int count, float I_bpsk[], float Q_bpsk[]){
        for (int i = 0; i < count; i++){
            if (bits[i] == 0){    
                I_bpsk[i] = 1.0;   
                Q_bpsk[i] = 0.0;
            }
            else  {
                I_bpsk[i] = -1.0;    
                Q_bpsk[i] = 0.0;
            }
        }
    }   

void upsampling(int duration, int count, float I_bpsk[], float Q_bpsk[], float I_upsampled[], float Q_upsampled[]){
    int index = 0;
    for (int i = 0; i < count; i++){   //новый массив увеличенный в 10 раз
        for (int sample = 0; sample < duration; sample++){
            //if (sample == duration - 1){
            if (sample == duration - 10){
                I_upsampled[index] = I_bpsk[i];
                Q_upsampled[index] = Q_bpsk[i];
            }
            else{
                I_upsampled[index] = 0.0;
                Q_upsampled[index] = 0.0;
            }
            index++;
        }
    }
}

void convolution(int sample, float I_upsampled[], float Q_upsampled[], float I_filtred[], float Q_filtred[]){
    int length = 10;
    float mask[10] = {1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0};

    int blocks = sample / length;

    for (int block = 0; block < blocks; block++) {
        int start_index = block * length;
        for (int i = 0; i < length; i++) {
            float sum_I = 0;
            float sum_Q = 0;
            
            for (int j = 0; j < length; j++) {
                int signal_index = start_index + j;
                if (signal_index < sample) {
                    int mask_index = length - 1 - j;  
                    sum_I += I_upsampled[signal_index] * mask[mask_index];
                    sum_Q += Q_upsampled[signal_index] * mask[mask_index];
                }
            }
            
            I_filtred[start_index + i] = sum_I;
            Q_filtred[start_index + i] = sum_Q;
        }
    }
}

void sdvig(int sample, float I_filtred[], float Q_filtred[]){
    for (int i = 0; i < sample; i++){
        I_filtred[i] = I_filtred[i] * 2047 * 16; 
        Q_filtred[i] = Q_filtred[i] * 2047 * 16;
    } 
}

void to_buff(float I_filtred[], float Q_filtred[], int16_t tx_buff[], int sample){
    for (int i = 0; i < sample; i++){
        tx_buff[2*i] = (int16_t)I_filtred[i];
        tx_buff[2*i + 1] = (int16_t)Q_filtred[i];
    }
}

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
    SoapySDRKwargs_set(&args, "loopback", "1");             
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

    SoapySDRDevice_setGain(sdr, SOAPY_SDR_RX, 1, 10.0);    //Чувствительность приемника
    SoapySDRDevice_setGain(sdr, SOAPY_SDR_TX, 1, -90.0);   //Усиление передатчика

    size_t channel_count = sizeof(channels) / sizeof(channels[0]);
    
    SoapySDRStream *rxStream = SoapySDRDevice_setupStream(sdr, SOAPY_SDR_RX, SOAPY_SDR_CS16, channels, channel_count, NULL);
    SoapySDRStream *txStream = SoapySDRDevice_setupStream(sdr, SOAPY_SDR_TX, SOAPY_SDR_CS16, channels, channel_count, NULL);

    SoapySDRDevice_activateStream(sdr, rxStream, 0, 0, 0); 
    SoapySDRDevice_activateStream(sdr, txStream, 0, 0, 0); 
    
    size_t rx_mtu = SoapySDRDevice_getStreamMTU(sdr, rxStream);
    size_t tx_mtu = SoapySDRDevice_getStreamMTU(sdr, txStream);

    int16_t tx_buff[2 * tx_mtu] = {0}; 
    int16_t rx_buff[2 * rx_mtu] = {0};

    int bits[] = {0,1,0,1,1,0,1,1,0,1};
    int count = 10;      //количество битов
    int duration = 10;   //количество семплов на символ
    int sample = count * duration;
    
    float I_bpsk[count];
    float Q_bpsk[count];

    float I_upsampled[sample];
    float Q_upsampled[sample];

    float I_filtred[sample];
    float Q_filtred[sample];

    for (int i = 0; i < sample; i++) {
        I_filtred[i] = 0.0f;
        Q_filtred[i] = 0.0f;
    }

    to_bpsk(bits, count, I_bpsk, Q_bpsk);
    upsampling(duration, count, I_bpsk, Q_bpsk, I_upsampled, Q_upsampled);
    convolution(sample, I_upsampled, Q_upsampled, I_filtred, Q_filtred);
    sdvig(sample, I_filtred, Q_filtred);
    to_buff(I_filtred, Q_filtred, tx_buff, sample);


    long long timeNs; //timestamp for receive buffer

    long long tx_time = timeNs + (4 * 1000 * 1000); // на 4 [мс] в будущее

    // Добавляем время, когда нужно передать блок tx_buff, через tx_time -наносекунд
    for(size_t i = 0; i < 8; i++)
    {
        uint8_t tx_time_byte = (tx_time >> (i * 8)) & 0xff;
        tx_buff[2 + i] = tx_time_byte << 4;
    }
    
    const long  timeoutUs = 400000;
    long long last_time = 0;
    size_t iteration_count = 10;

    FILE *file = fopen("txdata.pcm", "wb");
    //получение и отправка сэмплов
    for (size_t buffers_read = 0; buffers_read < iteration_count; buffers_read++)
    {
        void *rx_buffs[] = {rx_buff};
        int flags;        
        long long timeNs; 

        // считали буффер RX, записали его в rx_buffer
        int sr = SoapySDRDevice_readStream(sdr, rxStream, rx_buffs, 1920, &flags, &timeNs, timeoutUs);
        fwrite(rx_buff, 2* rx_mtu * sizeof(int16_t), 1, file);
        // Смотрим на количество считаных сэмплов, времени прихода и разницы во времени с чтением прошлого буфера
        printf("Buffer: %lu - Samples: %i, Flags: %i, Time: %lli, TimeDiff: %lli\n", buffers_read, sr, flags, timeNs, timeNs - last_time);
        last_time = timeNs;

        long long tx_time = timeNs + (4 * 1000 * 1000); // на 4мс в будущее

        for(size_t i = 0; i < 8; i++)
        {
            uint8_t tx_time_byte = (tx_time >> (i * 8)) & 0xff;
            tx_buff[2 + i] = tx_time_byte << 4;
        }

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