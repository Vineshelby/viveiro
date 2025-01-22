#ifndef SERIAL_READ_FOR_DATA_MANAGER_H
#define SERIAL_READ_FOR_DATA_MANAGER_H
#include "data_types.hpp"
#include "peripheral_control.hpp"
#include <Arduino.h>

extern const int numModules;

const String IDS_HUMI_TEXT = "ID dos sensores de umidade de solo:";
const String IDS_TEMP_TEXT = "ID dos sensores de temperatura:";
const String INITIAL_TEXT_HUMI_IDs = "Registro dos IDs dos sensores de umidade de Solo(A1-A5), digite um por vez:";
const String INITIAL_IDS_TEMP_TEXT = "Registro dos IDs dos sensores de temperatura (D1-D5), digite um por vez:";
const String ID_CONFIRMATION_TEXT = "Deseja salvar os IDs lidos? (0)Digitar novamente (1)Salvar (2)Sair";

const String INITIAL_CALIBRATION_TEXT = "Calibração dos limites dos sensores de umidade, posicione os sensores na situação 0% de umidade de solo. (1)Iniciar (0)Cancelar";
const String CALIBRATION_TEXT = "Calibração do valores mínimos lidos, posicione os sensores na situação 100% de umidade de solo (1)Para continuar";
const String CALIBRATION_CONFIRMATION_TEXT = "Deseja salvar os parâmetros acima? (0)Ler novamente (1)Salvar (2)Sair";

const String INITIAL_WIFI_TEXT = "Iniciando gravação wi-fi, insira inicialmente o nome da rede:";
const String WIFI_TEXT = "Insira a senha da rede:";
const String WIFI_CONFIRMATION_TEXT = "Deseja salvar os parâmetros acima? (0)Ler novamente (1)Salvar (2)Sair";

const String INITIAL_API_TEXT = "Iniciando gravação API, insira inicialmente o login da API:";
const String API_TEXT = "Insira a senha de acesso da API:";
const String API_CONFIRMATION_TEXT = "Deseja salvar os parâmetros acima? (0)Ler novamente (1)Salvar (2)Sair";

const String INITIAL_LINK_TEXT = "Links para comunicação com a API.";
const String LINK_AUTH_TEXT = "Link para autenticação:";
const String LINK_SENSOR_READING_TEXT = "Link para envio das leituras dos sensores:";
const String LINK_VALVE_TEXT = "Link para consulta do estado da válvula:";
const String LINK_TIME_VALVE_TEXT = "Link para acessar o tempo de funcionamento da vávula:";
const String LINK_WATER_FLOW_TEXT = "Link para envio das leitura do fluxo de água:";
const String LINK_CONFIRMATION_TEXT = "Deseja salvar os Links lidos? (0)Digitar novamente (1)Salvar (2)Sair";

const String CLEAR_CONFIRMATION_TEXT = "Apagar todos os dados guardados? (0)Não (1)Sim";
const String TEXT_MENU = "(1)Mostrar Dados Atuais\n(2)Registrar IDs dos Sensores de Humidade\n(3)Registrar IDs dos Sensores de Temperatura\n(4)Inserir credenciais Wi-Fi\n(5)Inserir credenciais da API\n(6)Calibrar Sensores de Umidade de Solo\n(7)Inserir Links da API\n(8)Apagar Todos os Dados Salvos\nRemova o jumper e reinicie a placa para sair\n";
const String ABORT_SERIAL_READ = "Operação cancelada";
const String TEXT_ERRO_NVS = "Dados Salvos Não Encontrados";
const String POWER_MODE_TEXT = "(0)NORMAL MODE\n(1)CONFIG MODE";

class SerialIOManager
{
  private:
    unsigned long int timeoutArgSerial = 360000;
    Peripheral *peripheral;
    const bool max = true, min = true;
  public:
    HardwareSerial* serial;

    SerialIOManager(HardwareSerial* serialObj, Peripheral* peripheralObj) 
    {
      serial = serialObj;
      peripheral = peripheralObj;
    }

    void begin(int baudRate);
    void clearSerialBuffer();

    bool readUserIDs(Sensor sensor[], const String &INITIAL, const String &PRESENTATION);
    bool readHumiCalibration(Sensor sensor[]);
    bool readCredentials(Credentials &credentials, const String &initialText, const String &mainText, const String &confirmationText);
    bool readLinks(ApiLinks &apiLinks);
    bool confirmationClearAllStorage();

    void showIDsArray(Sensor sensor[]);
    void showCredentials(String &login);
    void showApiLinks(ApiLinks &apiLinks);
    void showCalibration(Sensor sensor[]);
    void showCurrentCalibrationValue(Sensor sensor[], bool op);

    void showAllData(Sensor sensorH[], Sensor sensorT[], Credentials &wifi, Credentials &api, ApiLinks &apiLinks);
    void menuConfig();
    int waitForInt(int max, int min);

    void errorNvs();
    void operationCancelled();
    bool waitforPowerMode();
};

#endif