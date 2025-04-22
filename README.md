# Interface Interativa com Joystick e Feedback Visual e Sonoro  
Este projeto implementa um sistema interativo com múltiplos periféricos utilizando a placa BitDogLab com Raspberry Pi Pico. O sistema opera com dois modos alternados por botões: controle de um ponto na tela OLED ou controle de brilho de uma matriz 5x5 de LEDs com efeito degradê circular. Também há feedback visual com um LED RGB em efeito arco-íris e sonoro com um buzzer ativado temporariamente.  

---

## **Configuração dos Pinos GPIO**

Os pinos GPIO da Raspberry Pi Pico estão configurados conforme a tabela abaixo:

| GPIO   | Componente      | Função                                                                 |
|--------|-----------------|------------------------------------------------------------------------|
| **27** | Joystick X      | Entrada analógica para leitura do eixo horizontal                     |
| **26** | Joystick Y      | Entrada analógica para leitura do eixo vertical                       |
| **5**  | Botão A         | Alterna entre os modos de operação                                    |
| **6**  | Botão B         | Ativa o buzzer por 2,5 segundos                                       |
| **10** | Buzzer          | Saída PWM para geração de som                                         |
| **11** | LED Verde       | Saída PWM para efeito arco-íris                                       |
| **12** | LED Azul        | Saída PWM para efeito arco-íris                                       |
| **13** | LED Vermelho    | Saída PWM para efeito arco-íris                                       |
| **14** | I2C SDA         | Linha de dados para comunicação I2C com o display OLED                |
| **15** | I2C SCL         | Linha de clock para comunicação I2C com o display OLED                |
| **7**  | Matriz 5x5      | Saída de dados da matriz de LEDs controlada via PIO                   |

---

## **Funcionamento do Código**

1. **Inicialização:**

   - Configura os pinos GPIO para os botões, joystick, buzzer, LED RGB e matriz de LEDs.
   - Inicializa a comunicação I2C para o display OLED.
   - Configura o PWM para o LED RGB e o buzzer.
   - Inicializa o periférico PIO para controle da matriz de LEDs.
   - Configura as interrupções nos botões A e B com debounce.

2. **Leitura do Joystick:**

   - O sistema realiza a leitura analógica dos eixos X e Y do joystick.
   - Os valores são utilizados para definir a posição de um ponto na tela OLED ou influenciar o brilho da matriz WS2812b.

3. **Modos de Operação:**

   - **Modo 1 – OLED:** o ponto se move na tela conforme o joystick.
   - **Modo 2 – Matriz:** LEDs da matriz 5x5 exibem um degradê circular de brilho com base na posição do joystick.
   - O botão **A** alterna entre os dois modos.

4. **Feedback Visual e Sonoro:**

   - O **LED RGB** exibe um efeito de transição contínua de cores (arco-íris), alternando entre pares de LEDs (vermelho/verde, verde/azul, azul/vermelho).
   - O **buzzer** é ativado por 2,5 segundos ao pressionar o botão **B**, usando alarme one-shot.

5. **Exibição no Display OLED:**

   - Em **modo 1**, é exibido apenas o ponto móvel.
   - Em **modo 2**, são mostrados:  
     - Valores do joystick  
     - Estado do buzzer (ON/OFF)  
     - Indicação de que o modo da matriz de LEDs está ativo

6. **Tratamento de Interrupção:**

   - Os botões A e B são tratados por interrupções com debounce de 200 ms.
   - Botão A muda o modo de operação.
   - Botão B aciona o buzzer.

