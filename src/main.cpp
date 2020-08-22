#include <Arduino.h>
#include <tag125reader.h>

/**************************************************/
/* Variáveis                                      */
/**************************************************/
uint32_t tag125_value;

/**************************************************/
/* Hardware Setup                                 */
/**************************************************/
void setup()
{
  cli();

  //Inicialização do Leitor 125
  tag125.setup();

  //Inicialização Serial
  Serial.begin(57600);

  //Leitura da tag 125
  tag125_value = 0;

  sei();

  //Impressão dos valores
  Serial.println("Versão: 125K");
  Serial.println("--- Teste Leitura Cartão ---");
  /*
  Serial.print("Modulo Modelo: 1");
  Serial.println("");
  Serial.println("Modelo 1 teclado / Modelo 0 passageiro");
  */
}

/**************************************************/
/* Loop Principal                                 */
/**************************************************/
void loop()
{
  if (tag125.decode(&tag125_value))
  {
    Serial.print("Value Reader: ");
    Serial.println(tag125_value);
  }
}