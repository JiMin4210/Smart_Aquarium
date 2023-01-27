# smart_aquarium

## 👋 프로젝트 소개
> **MQTT 통신, Web을 이용한 수족관 원격 제어 서비스**
- **프로젝트 기간** : 2021.10 ~ 2021.11

## :pushpin: 기대효과
- 지금까지의 온도, 오염도, 조도의 기록이 저장된 influxDB를 바탕으로 Grafana를 통해 그래프로 확인 가능한데 이는 **수족관 환경에 대한 분석이 쉬워지는 긍정적인 효과**를 가져옴
- 포트 포워딩을 통해 스마트 수족관 서버가 열리는 포트를 열어 놨기 때문에 **집이 아닌 곳에서 자신의 수족관의 상태를 확인** 할 수 있어 사용자가 **안심이 되는 효과**를 가져옴

## ⭐ 기능
- **NEO PIXEL**
    > 임계 조도센서 값을 설정 -> 임계 조도 값보다 낮아지면 정해진 LED 패턴 실행
<img src="https://user-images.githubusercontent.com/90883534/215114028-ee00b95a-abbf-4693-8361-d54be3913792.png" width="700" height="200"/>

- **towerpro mg90s(서보 모터)**
    > 먹이를 주는 주기 설정 -> Esp8266 시간 변수를 통해 일정 시간 도달 시 서보 모터 작동<br>
    > Feed 버튼을 통해 원하는 수족관에 먹이 바로 제공
<img src="https://user-images.githubusercontent.com/90883534/215114440-b4054a03-a73a-47bb-a208-a15508ce1ffd.png" width="700" height="200"/>

- **DS18B20 튜브 모듈 (물 온도 측정), SEN 0189 (오염도 측정)**
    > 임계 온도, 오염도를 초과 할 시 초록색 -> 노랑색 -> 빨간색 순서 위험도에 따라 색이 변함
<img src="https://user-images.githubusercontent.com/90883534/215115026-025acc4a-ef90-4d32-9c38-e0e82b8dbacd.png" width="700" height="200"/>

- **InfluxDB, Grafana**
    > InfluxDB에 저장된 오염도, 온도, 조도의 기록이 Grafana를 통해 사용자가 현재까지의 기록을 한눈에 볼 수 있음
<img src="https://user-images.githubusercontent.com/90883534/215115389-b7312ed7-a2fb-4402-8f69-b8f20e945ffb.png" width="700" height="200"/>

## 🔨 사용 보드
- **Esp8266**

## ⚡ 사용 기술
- **Docker** : MQTT, Influxdb, Grafana 서버 제작
- **HTTP** : Web 통신 프로토콜
- **WIFI** : 수족관을 원격 제어하기 위함

## 📝 사용 언어
- **HTML** : 수족관 Web 제작
- **C** : MQTT 통신, HTTP 통신, 수족관 제어를 위한 센서 제어

## 🔆 개발 환경
- **Linux(AWS EC2)** : Docker를 통한 서버 제작
- **Visual Studio Code**







