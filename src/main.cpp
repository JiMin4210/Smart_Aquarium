#include <Arduino.h>
#include <EEPROM.h>
#include <ssd1306.h> // oled에 웹 주소 출력 목적

#include <ESP8266WiFi.h> // 서버를 위한 헤더
#include <DNSServer.h>
#include <ESP8266mDNS.h>
#include <ESP8266WebServer.h>

#include <PubSubClient.h> // mqtt 헤더

#define NO_CAP 1

#define             EEPROM_LENGTH 1024
#define             RESET_PIN 0
char                eRead[30];
#if NO_CAP == 1
  char                ssid[30] = "U+Net9B20";
  char                password[30] = "DD6B001103";
  char                mqtt[30] = "54.90.184.120"; 
#else
  char                ssid[30];
  char                password[30];
  char                mqtt[30]; // 60번지까지 eeprom을 읽게 한다. 30칸 추가이용 read string + MQTT 서버 주소를 변수에 넣어줘야함.
#endif
const int           mqttPort = 1883;
const byte DNS_PORT = 53;

WiFiClient espClient;
PubSubClient client(espClient);
ESP8266WebServer    webServer(80);
DNSServer dnsServer;
SSD1306 display(0x3c, 4, 5, GEOMETRY_128_32);

void callback(char* topic, byte* payload, unsigned int length);
void handleRoot();
void handleNotFound();
void Save_Info();
void Bab();
void Photo();
String web_new();

#define BOARD_NUM 2

float temp[BOARD_NUM]; // 현재 온도
float critical_temp[BOARD_NUM] = {36,36}; // 임계 온도
int env[BOARD_NUM]; // 현재 오염도
int critical_env[BOARD_NUM]; // 임계 오염도
int val[BOARD_NUM]; // 조도센서 값
int critical_val[BOARD_NUM]; // 임계 조도
int feed_num[BOARD_NUM]; // 먹이 준 회수
int feed_cycle[BOARD_NUM]; // 먹이 주는 주기
char *status[BOARD_NUM];
char *status_color[3] = {"green","yellow","red"}; // 3가지 상태 존재.
char LED_status[BOARD_NUM][4] = {"0","0"};
char photo_url[2][80] = {"/","/"}; // 사진 url을 담는 용도 (올라온 사진이 없다면 그저 새로고침이 됨)

String sub_topic[2] = {"deviceid/Board_A/evt/#","deviceid/Board_B/evt/#"};
String pub_topic[2] = {"deviceid/Board_A/cmd/#","deviceid/Board_B/cmd/#"};

char photo[] = "" // 값 최신화를 하지 않고도 최신 사진을 바로 볼수 있게 한 것
"<!DOCTYPE html><html><head>"
"<title>META Tag  Refresh</title>"
"<meta http-equiv='content-type' content='text/html; charset=euc-kr'>"
"<meta http-equiv='refresh' content='0; url=%s'></head>";


char myWeb_01[] =""
    "<!DOCTYPE html><html>"
    "<head><meta charset=utf-8 /><title>AQUARIUM WEB</title>"
    "<link rel='preconnect' href='https://fonts.googleapis.com'>"
    "<link rel='preconnect' href='https://fonts.gstatic.com' crossorigin>"
    "<link href='https://fonts.googleapis.com/css2?family=Nanum+Pen+Script&display=swap' rel='stylesheet'>"
    "<style>body{background-image : url('https://vrthumb.clipartkorea.co.kr/2017/04/03/ti237a8402.jpg');background-repeat : no-repeat;background-size : cover;}"
    "@media(min-width:300px){div{width: 100%%; height: 60%%;}}@media(min-width:1000px){div{width: 90%%; height: 60%%;}}@media(min-width:1200px){div{width: 80%%; height: 60%%;}}"
    "@media(min-width:1500px){div{width: 65%%; height: 60%%;}}@media(min-width:1820px){div{width: 50%%; height: 50%%;}}"
    "div.left {width: 50%%; float: left;box-sizing: border-box;}"
    "div.right {width: 50%%; float: right;box-sizing: border-box;}"
    "div.parent{width: 77%%;height: 10%%;}div.first {float: left;width:50%%;box-sizing: border-box;}div.second{float: right;width:50%%;box-sizing: border-box;}</style></head>"
    "<body><center><p style='font-weight:700; font-size:100px; font-family:Nanum Pen Script; color:rgb(54, 121, 184); '>AQUARIUM WEB</p>"
    "<div><div class='left'><table border='3' width ='500' height='300' align = 'center'>"
    "<p style='font-weight:900'>수족관 상태</p>"
    "<th></th><th align = 'center' style='color:black'>Board_A</th><th align = 'center' style='color:black'>Board_B</th>"
    "<tr bgcolor=skyblue align = 'center' style='font-weight:700; color: black;'><td>현재 온도</td><td>%.1f</td><td>%.1f</td></tr>"
    "<tr align = 'center' style='font-weight:700; color: black; '><td>임계 온도</td><td>%.1f</td><td>%.1f</td></tr>"
    "<tr bgcolor=skyblue align = 'center' style='font-weight:700; color: black;'><td>현재 오염도</td><td>%d</td><td>%d</td></tr>"
    "<tr align = 'center' style='font-weight:700; color: black;'><td>임계 오염도</td><td>%d</td><td>%d</td></tr>"
    "<tr bgcolor=skyblue align = 'center' style='font-weight:700; color: black;'><td>현재 조도값</td><td>%d</td><td>%d</td></tr>";
    
char myWeb_02[] =""
  "<tr align = 'center' style='font-weight:700; color: black;'><td>임계 조도값(LED 상태)</td><td>%d(%s)</td><td>%d(%s)</td></tr>"
  "<tr align = 'center' style='font-weight:700; color: black;'><td>먹이 주는 주기(시간)</td><td>%d</td><td>%d</td></tr>"
    "<tr align = 'center' style='font-weight:700; color: black;'><td>먹이 준 회수</td><td>%d</td><td>%d</td></tr>"
    "<tr align = 'center' style='font-weight:700; color: black;'><td>현재 상태</td><td bgcolor='%s'></td><td bgcolor='%s'></td>" // 컬러 변경 필요
    "</table></div>"
    "<div class='right'>"
    "<form action='/Save_Info'>"
    "<p style='font-weight:900'>수족관 제어 값 설정</p>"
    "<label><input type='text' name='temp' placeholder='임계 온도' onblur='this.value=removeSpaces(this.value);'> </label>"
    "<label><input type='text' name='cycle' placeholder='먹이 주는 주기(시간 단위)'> </label><p></p>"
    "<label><input type='text' name='env' placeholder='임계 오염도'> </label>"
    "<label><input type='text' name='val' placeholder='임계 조도센서'> </label>"
    "<div class='parent'><div class='first'>"
    "<p><select name='LED' style='WIDTH: 133pt; HEIGHT: 16pt'><option value='0'>LED_OFF</option><option value='1'>LED 패턴 1</option>"
    "<option value='2'>LED 패턴 2</option><option value='3'>LED 패턴 3</option><option value='4'>LED 패턴 4</option></select></select></p></div>"
    "<div class='second'>"
    "<p><select name='BOARD' style='WIDTH: 133pt; HEIGHT: 16pt' ><option value='BOARD_A'>BOARD_A</option><option value='BOARD_B'>BOARD_B</option></select></p></div></div>"
    "<p></p><div class='parent'><div class='first'>"
    "<p><input type='submit' style='WIDTH: 133pt; HEIGHT: 30pt' value='값 저장'></p></div></form>"
    "<div class='second'><form action='/'><p><input type='submit' style='WIDTH: 133pt; HEIGHT: 30pt' value='값 최신화'></p></form></div></div>"
    "<div class='parent'><div class='first'><form action='/Bab'>"
    "<p><input type='submit' style='WIDTH: 133pt; HEIGHT: 30pt' name='Board' value='Feed_A'></p></div> "
    "<div class='second'><p><input type='submit' style='WIDTH: 133pt; HEIGHT: 30pt' name='Board' value='Feed_B'></p> </div></div></form>";
   
 char myWeb_03[] =""  
    "<p> </p><div class='parent'><div class='first'><form action='/Photo'>"
    "<p><input type='submit' style='WIDTH: 133pt; HEIGHT: 30pt' name='Photo' value='Photo_A'></p></div></form>"
    "<div class='second'><form action='/Photo'><p><input type='submit' style='WIDTH: 133pt; HEIGHT: 30pt' name='Photo' value='Photo_B'></p></form></div></div>"
     "</div></div></center></body>"
    "<script>function removeSpaces(string) {return string.split(' ').join('');}</script></html>";


String responseHTML = ""
    "<!DOCTYPE html><html><head><title>CaptivePortal</title></head><body><center>"
    "<p style='font-weight:700'>WiFi & MQTT Setup</p>" // 글자 굵기, 텍스트 변경 
    "<form action='/save'>" // get 방식으로 url/save?name=value&password=value 보냄 (이후 arg로 받아낸다.) 
    "<p'><input type='text' name='ssid' placeholder='SSID' onblur='this.value=removeSpaces(this.value);'></p>"
    "<p><input type='text' name='password' placeholder='WLAN Password'></p>"
    "<p><input type='text' name='mqtt' placeholder='MQTT Server'></p>" // MQTT 서버 받기 위해 추가한 부분
    "<p><input type='submit' value='Submit'></p></form>"
    "<p style='font-weight:700'>WiFi & MQTT Setup Page</p></center></body>" // 글자 굵기, 텍스트 변경
    "<script>function removeSpaces(string) {"
    "   return string.split(' ').join('');"
    "}</script></html>";

// Saves string to EEPROM
void SaveString(int startAt, const char* id) { 
    for (byte i = 0; i <= strlen(id); i++) {
        EEPROM.write(i + startAt, (uint8_t) id[i]);
    }
    EEPROM.commit();
}

// Reads string from EEPROM
void ReadString(byte startAt, byte bufor) {
    for (byte i = 0; i <= bufor; i++) {
        eRead[i] = (char)EEPROM.read(i + startAt);
    }
}

void save(){
    Serial.println("button pressed");
    Serial.println(webServer.arg("ssid"));
    Serial.println(webServer.arg("mqtt"));
    SaveString( 0, (webServer.arg("ssid")).c_str());
    SaveString(30, (webServer.arg("password")).c_str());
    SaveString(60, (webServer.arg("mqtt")).c_str()); //  MQTT 서버 받기 위해 추가한 부분
    webServer.send(200, "text/plain", "OK");
    ESP.restart();
}

void configWiFi() {
    display.init();
    display.setFont(ArialMT_Plain_16);
    display.flipScreenVertically();
    display.drawString(0,10,"Captive potal"); // 연결중임을 OLED에 표시
    display.display();
    IPAddress apIP(192, 168, 1, 1);
    
    WiFi.mode(WIFI_AP);
    WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
    WiFi.softAP("Aquarium_cap");     // change this to your portal SSID
    
    dnsServer.start(DNS_PORT, "*", apIP);

    webServer.on("/save", save);

    webServer.onNotFound([]() {
        webServer.send(200, "text/html", responseHTML);
    });
    webServer.begin();
    while(true) {
        dnsServer.processNextRequest(); 
        webServer.handleClient();
        yield();
    }
}

void load_config_wifi() {
    ReadString(0, 30);
    if (!strcmp(eRead, "")) {
        Serial.println("Config Captive Portal started");
        configWiFi();
    } else {
        Serial.println("IOT Dev2ice started");
        strcpy(ssid, eRead);
        ReadString(30, 30);
        strcpy(password, eRead);
        ReadString(60, 30); // MQTT 서버 받기 위해 추가한 부분
        strcpy(mqtt, eRead);
    }
}

IRAM_ATTR void GPIO0() {
    SaveString(0, ""); // blank out the SSID field in EEPROM - ""넣어주면 이후 메모리 다 초기화인가보다
    ESP.restart();
}

void setup() {
    Serial.begin(115200);
    EEPROM.begin(EEPROM_LENGTH);
    pinMode(RESET_PIN, INPUT_PULLUP);
    attachInterrupt(RESET_PIN, GPIO0, FALLING);
    display.init(); // 처음 화면 초기화

    while(!Serial);
    Serial.println();

    if(!NO_CAP) // captive potal 안쓰고 실험 할 때
      load_config_wifi(); // load or config wifi if not configured

    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    Serial.println("");

    // Wait for connection -OLED 사용 불가
    display.init();
    display.setFont(ArialMT_Plain_16);
    display.flipScreenVertically();
    display.drawString(0,0,ssid);
    display.drawString(0,14,"connecting"); // 연결중임을 OLED에 표시
    int i = 0;
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
        display.drawString(i*3+75,14,".");
        display.display();
        if(i++ > 100 && !NO_CAP) {
            configWiFi();
        }
    }
    Serial.print("Connected to "); Serial.println(ssid);
    Serial.print("IP address: "); Serial.println(WiFi.localIP());
    Serial.print("mqtt address: "); Serial.println(mqtt);

    //DNS
    dnsServer.start(DNS_PORT, "*", WiFi.localIP());

    // mqtt
    client.setServer(mqtt, mqttPort);
    client.setCallback(callback);

    while (!client.connected()) {
        Serial.println("Connecting to MQTT...");
        if (client.connect("control_board")) {
            Serial.println("connected");
            Serial.println("-------Sub Start-------");
            String topic[4] = {"temp","env","val","photo"}; // 보드A,B 각각 3가지 요소 구독
            for(int i = 0; i<2; i++)
            {
              for(int x = 0; x<4; x++)
              {
                String buf = sub_topic[i];
                buf.replace("#",topic[x]);
                client.subscribe(buf.c_str());
                Serial.println(buf.c_str());
              }
            }     
            Serial.println("-------Sub End-------");
        } else {
            Serial.print("failed with state "); Serial.println(client.state());
            delay(2000);
        }
    }

    // 웹페이지
    webServer.on("/", handleRoot);
    webServer.on("/Save_Info",Save_Info);
    webServer.on("/Bab",Bab);
    webServer.on("/Photo",Photo);
    webServer.onNotFound(handleNotFound);
    webServer.begin(); // 서버 시작
    Serial.println("HTTP server started");

    // OLED - mDNS가 안돼서 OLED를 통해 IP주소를 받고 웹 접속 - OLED 사용 불가 (EEPROM이랑 동시사용 안됨)
    display.init();
    display.setFont(ArialMT_Plain_16);
    display.flipScreenVertically();
    display.drawString(0,10,WiFi.localIP().toString());
    display.display();
    Serial.println("OLED OUTPUT IP");

    Serial.println("Runtime Starting");  
}

void loop() {
    client.loop();
    dnsServer.processNextRequest();
    webServer.handleClient();
}

void callback(char* topic, byte* payload, unsigned int length) {
    char buf[80] = {0,};
    Serial.println(topic);
    for(int i = 0; i<length; i++)
      buf[i] = (char)payload[i];

    if(buf[0] != NAN)
    {
      int board_num = 0;
      if(strstr(topic,"Board_B"))
        board_num = 1;
      
      if(strstr(topic,"temp"))
        temp[board_num] = atof(buf);
      else if(strstr(topic,"val"))
        val[board_num] = atoi(buf);
      else if(strstr(topic,"env"))
        env[board_num] = atoi(buf);
      else if(strstr(topic,"photo"))
        strcpy(photo_url[board_num],buf); 
    }
}

//----------------------------------------------------------웹 관련 함수----------------------------------------------------------------------------------
void Save_Info(){
  int board_num = 0;
  String web_arg;
  String pub = pub_topic[0];
  if(!strcmp(webServer.arg("BOARD").c_str(),"BOARD_B")) // 보드 선택
  {
    board_num = 1;
    pub = pub_topic[1];
  }

  Serial.println("-------Pub Start-------");

  web_arg = webServer.arg("temp"); // webarg함수를 좀 많이 호출하기에 저장해두고 사용하면 더 빠름
  if(web_arg != NULL) // 빈칸으로 sumit하면 NULL 반환 - 아무것도 안써서 값 없어지는거 방지
  {
    String buf = pub; // replace함수가 리턴이 아닌 본래의 값을 바꾸기에 buf를 하나 선언해줌으로써 pub이 변하는것을 방지
    critical_temp[board_num] = atof(web_arg.c_str()); // 임계 온도 값
    buf.replace("#","temp");
    Serial.println(buf);
    client.publish(buf.c_str(),web_arg.c_str());
  }
 
  web_arg = webServer.arg("env");
  if(web_arg != NULL)
  {
    String buf = pub;
    critical_env[board_num] = atoi(web_arg.c_str()); // 임계 오염도
    buf.replace("#","env");
    Serial.println(buf);
    client.publish(buf.c_str(),web_arg.c_str());
  }

  web_arg = webServer.arg("val");
  if(web_arg != NULL)
  {
    String buf = pub;
    critical_val[board_num] = atoi(webServer.arg("val").c_str()); // 임계온도 조도센서
    buf.replace("#","val");
    Serial.println(buf);
    client.publish(buf.c_str(),web_arg.c_str());
  }

  web_arg = webServer.arg("cycle");
  if(web_arg != NULL)    
  {
    String buf = pub;
    feed_cycle[board_num] = atoi(web_arg.c_str()); // 밥주는 주기
    buf.replace("#","cycle");
    Serial.println(buf);
    client.publish(buf.c_str(),web_arg.c_str());
  }

  web_arg = webServer.arg("LED"); 
  if(web_arg != NULL)    
  {
    String buf = pub;
    strcpy(LED_status[board_num],web_arg.c_str()); // LED_ON_OFF 제어(패턴 제어 0 = off ,1~4 = LED패턴 설정)
    buf.replace("#","LED");
    Serial.println(buf);
    client.publish(buf.c_str(),web_arg.c_str());
  }
  
  Serial.println("-------Pub End-------");
  webServer.send(200,"text/html", web_new());
}

void Bab(){
  String buf;
  int board_num = 0;
  if(!strcmp(webServer.arg("Board").c_str(),"Feed_B")) // strcmp에서 같을 때 반환값 = 0
    board_num = 1;

  feed_num[board_num]++;
  buf = pub_topic[board_num]; // 결정된 보드의 topic을 복사한다.
  buf.replace("#","bab");
  Serial.println("-------Pub Start-------");
  Serial.println(buf);
  client.publish(buf.c_str(),"1");
  Serial.println("-------Pub End-------");
  webServer.send(200,"text/html", web_new());
}

void handleRoot(){
  
  webServer.send(200,"text/html", web_new());
}

void handleNotFound(){
  String message = "Not Found";
  webServer.send(404,"text/html",message);
}

void Photo(){
  char buf[1000] = {0,};
  int board_num = 0;
  if(!strcmp(webServer.arg("Photo").c_str(),"Photo_B")) // 보드 선택
    board_num = 1;


  sprintf(buf,photo,photo_url[board_num]);
  
  webServer.send(200,"text/html", buf);
}

String web_new()
{
  for(int i = 0; i<2; i++)
  {
    int count = 0;
    if(temp[i]>critical_temp[i])
      count++;
    if(env[i]>critical_env[i])
      count++;
    status[i] = status_color[count];
    if(!strcmp(LED_status[i],"0"))
      strcpy(LED_status[i],"OFF");
  }
  

  char buf[2500] = {0,};
  sprintf(buf,myWeb_01,temp[0],temp[1],critical_temp[0],critical_temp[1],env[0],env[1],critical_env[0],critical_env[1],val[0],val[1]);// 인수가 10개 이상?이면 잘 안들어가서 나눠줌
  String message = String(buf);                                                                                                       // %까지 전달하기 위해선 %%를 써줘야함.
  sprintf(buf,myWeb_02,critical_val[0],LED_status[0],critical_val[1],LED_status[1],feed_cycle[0],feed_cycle[1],feed_num[0],feed_num[1],status[0],status[1]); 
  message += String(buf); 
  sprintf(buf,myWeb_03); 
  message += String(buf);                                                                                                         
  return message;
}
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------



