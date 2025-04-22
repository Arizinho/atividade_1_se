#include <stdio.h>
#include <math.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/pwm.h"
#include "hardware/i2c.h"
#include <stdlib.h>
#include "inc/ssd1306.h"
#include "inc/font.h"
#include "hardware/timer.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"

//arquivo .pio
#include "led_matrix.pio.h"

//macro dos pinos utilizados
#define BUTTON_A 5
#define BUTTON_B 6

#define JOYSTICK_X 27
#define JOYSTICK_Y 26
#define OUT_PIN 7

#define LED_RED 13
#define LED_GREEN 11
#define LED_BLUE 12

#define BUZZER 10

#define LIMIT 2.05*4905/4

//booleanos de controle
volatile bool buzzer_on = 0, mode = 0;

//matriz de intensidade dos LEDs 5x5
uint8_t desenho[25]  =  {0, 0, 0, 0, 0,
                        0, 0, 0, 0, 0,
                        0, 0, 0, 0, 0,
                        0, 0, 0, 0, 0,
                        0, 0, 0, 0, 0};

//macro comunicação i2c display oled
#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15
#define endereco 0x3C

ssd1306_t ssd; // Inicializa a estrutura do display

//protótipos de funções
void gpio_irq_handler(uint gpio, uint32_t events);
void button_init();
void joystick_init();
void joystick_read_value(uint16_t *analog_x, uint16_t *analog_y);
void i2c_oled_init();
void configura_pwm();
int64_t turn_off_callback(alarm_id_t id, void *user_data);
void rainbow_led();
uint8_t intensidade(uint16_t x0,uint16_t y0,uint8_t x1,uint8_t y1);
uint32_t matrix_rgb(uint8_t b, uint8_t r, uint8_t g);
void desenho_pio(uint8_t *desenho, PIO pio, uint sm);

int main(){
    uint8_t count = 0;
    bool first = 1;
    char texto_x[5], texto_y[5];
    uint32_t lasttime = 0;
    uint16_t analog_x, analog_y, oled_x, oled_y;

    //inicializa periféricos
    stdio_init_all();
    i2c_oled_init();
    button_init();
    configura_pwm();
    joystick_init();    

    //habilita interrupção por botão
    gpio_set_irq_enabled_with_callback(BUTTON_A, GPIO_IRQ_EDGE_FALL, 1, gpio_irq_handler);
    gpio_set_irq_enabled_with_callback(BUTTON_B, GPIO_IRQ_EDGE_FALL, 1, gpio_irq_handler);

    //inicialização matriz de leds
    PIO pio = pio0; //seleciona a pio0
    set_sys_clock_khz(128000, false); //coloca a frequência de clock para 128 MHz, facilitando a divisão pelo clock
    uint offset = pio_add_program(pio, &pio_matrix_program);
    uint sm = pio_claim_unused_sm(pio, true);
    pio_matrix_program_init(pio, sm, offset, OUT_PIN);

    while (true) {
        //chama função de brilho no led rgb
        rainbow_led();

        //lê valor do analógico
        joystick_read_value(&analog_x, &analog_y);

        //converte valor analógico em posição x,y do display
        oled_x = (analog_x)*119/4095 + 1;
        oled_y = (4095-analog_y)*55/4095 + 1;

        //modo de controle de ponto por joystick na tela oled
        if(!mode){
            ssd1306_fill(&ssd, false); //limpa tela oled
            ssd1306_draw_dot(&ssd, oled_x, oled_y); //desenha ponto de acordo com movimento do joystick
            ssd1306_send_data(&ssd); //envia dados para tela
            if(first){
                for (int16_t i = 0; i < 25; i++) {
                    pio_sm_put_blocking(pio, sm, matrix_rgb(0, 0, 0));
                }
                first = 0;
            }
        }

        //modo de controle de leds (estilo degradee) na matriz de leds 5x5 utilizando joystick
        //e mostra informações do sistema na tela oled
        else{
            if (!first){first = 1;}
            for (int16_t c = 0; c < 5; c++) {
                for (int16_t l = 0; l < 5; l++){
                    if (c%2==0)
                    {
                        desenho[count] = intensidade(analog_x-analog_x%32,4095-(analog_y-analog_y%32),l,c);
    
                    }else{
                        desenho[count] = intensidade(analog_x-analog_x%32,4095-(analog_y-analog_y%32),4-l,c);
                    }
                    count += 1;
                }
            }
            count = 0;
            desenho_pio(desenho, pio, sm);

            ssd1306_fill(&ssd, false); //limpa tela oled
            ssd1306_draw_string(&ssd,  "Modo matriz de LED ON",0, 0);
            
            if(buzzer_on){
                ssd1306_draw_string(&ssd,  "BUZZER ON",0, 24); 
            }
            else{
                ssd1306_draw_string(&ssd,  "BUZZER OFF",0, 24);
            }

            sprintf(texto_x, "%i", (analog_x-analog_x%32));
            sprintf(texto_y, "%i", (analog_y-analog_y%32));
            
            ssd1306_draw_string(&ssd, "VALOR", 0, 40);
            ssd1306_draw_string(&ssd, "ADC", 44, 40);
            ssd1306_draw_string(&ssd, "X", 72, 40);
            ssd1306_draw_string(&ssd, texto_x, 90, 40);

            ssd1306_draw_string(&ssd, "VALOR", 0, 55);
            ssd1306_draw_string(&ssd, "ADC", 44, 55);
            ssd1306_draw_string(&ssd, "Y", 72, 55);
            ssd1306_draw_string(&ssd, texto_y, 90, 55);

            ssd1306_send_data(&ssd); //envia dados para tela
            
        }
        if(to_ms_since_boot(get_absolute_time())-lasttime > 1000){
            lasttime = to_ms_since_boot(get_absolute_time());
            if(mode){printf("Modo de controle de matriz de LED por Joystick ON\n");}
            else{printf("Modo de controle de matriz de LED por Joystick OFF\n");}

            if(buzzer_on){printf("BUZZER: ON\n");}
            else{printf("BUZZER: OFF\n");}
            
            printf("Leitura ADC do Joystick em x: %i\n", analog_x);
            printf("Leitura ADC do Joystick em y: %i\n", analog_y);
            printf("--------------------------------------------------\n\n");
        }

        sleep_ms(20);
    }
}

//função para tratar interrupção no botão
void gpio_irq_handler(uint gpio, uint32_t events){
    //variável para guardar tempo da ocorrência da última interrupção
    static uint32_t last_time = 0;

    //compara diferença entre tempo atual e da última interrupção efetiva (debounce por delay de 0,2 s) 
    if(to_ms_since_boot(get_absolute_time())-last_time > 200){

        //alterna entre controle da tela oled ou matriz 5x5 de leds pelo joystick
        if (gpio == BUTTON_A){
            last_time = to_ms_since_boot(get_absolute_time()); //salva valor do tempo de ocorrência da última interrupção
            mode = !mode;
        }

        //ativa buzzer por 2,5 s
        else if (gpio == BUTTON_B){
            last_time = to_ms_since_boot(get_absolute_time()); //salva valor do tempo de ocorrência da última interrupção
            pwm_set_gpio_level(BUZZER, 1000);
            if (!buzzer_on){
                alarm_id_t alarm = add_alarm_in_ms(2500, turn_off_callback, NULL, false);
            }
            buzzer_on = 1;
        }
    }
}

//função para configurar os botões
void button_init(){
    //configura o botão A
    gpio_init(BUTTON_A);
    gpio_set_dir(BUTTON_A, GPIO_IN);
    gpio_pull_up(BUTTON_A);

    //configura o botão B
    gpio_init(BUTTON_B);
    gpio_set_dir(BUTTON_B, GPIO_IN);
    gpio_pull_up(BUTTON_B);
}

//função para inicializar pinos do joystick
void joystick_init(){
    //inicializa analógico
    adc_init();
    adc_gpio_init(JOYSTICK_X);
    adc_gpio_init(JOYSTICK_Y);
}

//função para fazer leitura analógica do joystick
void joystick_read_value(uint16_t *analog_x, uint16_t *analog_y){
    //lê movimento em horizontal
    adc_select_input(1);
    *analog_x = adc_read();

    //lê movimento vertical
    adc_select_input(0);
    *analog_y = adc_read();
}

//função para inicializar comunicação I2C com display oled
void i2c_oled_init(){
    // I2C Initialisation. Using it at 400Khz.
    i2c_init(I2C_PORT, 400 * 1000);

    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C); // Set the GPIO pin function to I2C
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C); // Set the GPIO pin function to I2C
    gpio_pull_up(I2C_SDA); // Pull up the data line
    gpio_pull_up(I2C_SCL); // Pull up the clock line
    ssd1306_init(&ssd, WIDTH, HEIGHT, false, endereco, I2C_PORT); // Inicializa o display
    ssd1306_config(&ssd); // Configura o display
    ssd1306_send_data(&ssd); // Envia os dados para o display

    // Limpa o display. O display inicia com todos os pixels apagados.
    ssd1306_fill(&ssd, false);
    ssd1306_send_data(&ssd);
}

//configara pwm do led rgb e buzzer
void configura_pwm(){
    //habilitar o pinos GPIO como PWM
    gpio_set_function(LED_RED, GPIO_FUNC_PWM); 
    gpio_set_function(LED_BLUE, GPIO_FUNC_PWM);
    gpio_set_function(LED_GREEN, GPIO_FUNC_PWM);
    gpio_set_function(BUZZER, GPIO_FUNC_PWM);

    //define o divisor de clock do PWM 
    pwm_set_clkdiv(5, 128.0); 
    pwm_set_clkdiv(6, 128.0);

    //definir o valor de wrap (vai contar até 20000 ciclos -> 19999 + 1)
    pwm_set_wrap(5, 1999); 
    pwm_set_wrap(6, 1999);

    //defini o duty cycle inicial
    pwm_set_gpio_level(BUZZER, 0); 
    pwm_set_gpio_level(LED_RED, 400);
    pwm_set_gpio_level(LED_BLUE, 0);
    pwm_set_gpio_level(LED_GREEN, 0);

    //habilita o pwm
    pwm_set_enabled(5, true); 
    pwm_set_enabled(6, true);
}

//alarme one-shot para desligar buzzer
int64_t turn_off_callback(alarm_id_t id, void *user_data) {
    pwm_set_gpio_level(BUZZER, 0); //defini o duty cycle para 0%
    buzzer_on = 0;
    return 0;
}

//função para realizar efeito arco-íris no led rgb com PWM
void rainbow_led(void){

    static uint16_t level = 10;
    static uint8_t leds_pair = 0;
    static bool controle = 1;

    //alterna intensidades do led rgb 2 a 2 cores -> vermelho/verde, verde/azul e azul/vermelho
    switch (leds_pair)
    {
    case 0:
        if (controle){
            pwm_set_gpio_level(LED_GREEN, level);
        }
        else if(!controle){
            pwm_set_gpio_level(LED_RED, level);
        }
        break;

    case 1:
        if (controle){
            pwm_set_gpio_level(LED_BLUE, level);
        }
        else if(!controle){
            pwm_set_gpio_level(LED_GREEN, level);
        }
        break;

    case 2:
        if (controle){
            pwm_set_gpio_level(LED_RED, level);
        }
        else if(!controle){
            pwm_set_gpio_level(LED_BLUE, level);
        }
        break;
    }

    if (level == 400 || level == 0){
        controle = !controle;
        if (controle){
            leds_pair = (leds_pair + 1)%3;
        }
    }

    level += (controle * 2 - 1) * 10;
}

//calcula a intensidade dos leds da matriz de acordo com a distância relativa à posição do joystick
uint8_t intensidade(uint16_t x0,uint16_t y0,uint8_t x1,uint8_t y1){
    uint16_t distance;
    
    //calcula distância
    distance = sqrt(pow((x1*4095)/4 - x0,2)+pow((y1*4095)/4 - y0,2));

    //apaga leds com distância relativa maior que o valor LIMIT de distância da posição do joystick 
    if (distance > (LIMIT)){
        return 0;
    }

    //retorna intensidade do led da matriz
    //dá mais enfase no brilho quanto menor a distância entre o led e a posição do joystick
    else {
        return 255*pow((LIMIT-distance)/(LIMIT),3)*0.2;
    }
}

//rotina para definição da intensidade de cores do led na matriz 5x5
uint32_t matrix_rgb(uint8_t b, uint8_t r, uint8_t g)
{
  return (g << 24) | (r << 16) | (b << 8);
}

//rotina para acionar a matrix de leds - ws2812b
void desenho_pio(uint8_t *desenho, PIO pio, uint sm){
    for (int16_t i = 0; i < 25; i++) {
        pio_sm_put_blocking(pio, sm, matrix_rgb(desenho[24-i], 0, 0));
    }
}
