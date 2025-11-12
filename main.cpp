#include <SoapySDR/Device.h>   
#include <SoapySDR/Formats.h>  
#include <stdlib.h>            
#include <stdint.h>
#include <complex.h>
#include <stdio.h>

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
            printf("(%.1f,%.1f) ", I_bpsk[i], Q_bpsk[i]);
        }
        printf("\n");
    }   

void upsampling(int duration, int count, float I_bpsk[], float Q_bpsk[], float I_upsampled[], float Q_upsampled[]){
    int index = 0;
    for (int i = 0; i < count; i++){   //новый массив увеличенный в 10 раз
        for (int sample = 0; sample < duration; sample++){
            if (sample == duration - 1){
            //if (sample == duration - 9){
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

void filter(int sample, float I_upsampled[], float Q_upsampled[], float I_filtred[], float Q_filtred[]){
    int length = 10;

    for (int i = 0; i < sample; i++){
        if (i % length == length - 1){
            float current_I = I_upsampled[i];
            float current_Q = Q_upsampled[i];

            for (int j = 0; j < length; j++){
                I_filtred[i - j] = current_I;
                Q_filtred[i - j] = current_Q;
            }
        }
    }
} // Копируем каждый 10-ый элемент в предыдущие позиции

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
    int bits[] = {0,1,0,1,1,0,1,1,0,1};
    int count = 10; //количество битов
    int duration = 10; //количество семплов на символ
    int sample = count * duration;
    
    float I_bpsk[count];
    float Q_bpsk[count];

    float I_upsampled[sample];
    float Q_upsampled[sample];

    float I_filtred[sample];
    float Q_filtred[sample];

    to_bpsk(bits, count, I_bpsk, Q_bpsk);

    upsampling(duration, count, I_bpsk, Q_bpsk, I_upsampled, Q_upsampled);

    filter(sample, I_upsampled, Q_upsampled, I_filtred, Q_filtred);

    sdvig(sample, I_filtred, Q_filtred);
    
    int16_t tx_buff[2*1920];

    to_buff(I_filtred, Q_filtred, tx_buff, sample);

    //первые 8 ячеек для timestamp
    for(size_t i = 0; i < 2; i++){
        tx_buff[0 + i] = 0xffff;
        // 8 x timestamp words
        tx_buff[10 + i] = 0xffff;
    }
    long long timeNs; //timestamp for receive buffer
     // Переменная для времени отправки сэмплов относительно текущего приема
    long long tx_time = timeNs + (4 * 1000 * 1000); // на 4 [мс] в будущее

    // Добавляем время, когда нужно передать блок tx_buff, через tx_time -наносекунд
    for(size_t i = 0; i < 8; i++)
    {
        uint8_t tx_time_byte = (tx_time >> (i * 8)) & 0xff;
        tx_buff[2 + i] = tx_time_byte << 4;
    }
}

