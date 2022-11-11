#include <SPI.h>             //Thư viện giao tiếp SPI
#include <MFRC522.h>        //Thư viện giao tiếp RFID
#include <Servo.h>          //Thư viện servo

#define SS_PIN 10     //Chân SDA RC522
#define RST_PIN 9     //Chân RST RC522

#define CD1 5      //Nút nhấn enable/disabe chế độ đọc thẻ
#define CD2 4      //Nút nhấn mở cửa không dùng thẻ
#define servo 7    //Chân servo
#define buzz 3     //Chân buzzer

uint8_t goc , id[4] ;
bool is_open = false, is_enable = false;    //is_open: trạng thái cửa mở nếu true và is_enable: trạng thái cho phép đọc thẻ nếu true
Servo myServo;

MFRC522 rfid(SS_PIN, RST_PIN);

uint8_t m7d[] = {0x2b, 0x11};    //giai ma led 7 doan (anode chung) 0x2b->n , 0x11->y

void write7d(char a = ' ') {                        //Mặc định truyền vào hàm chữ "n"
  if (a == 'n') {                                   //Hiển thị chữ "n"
    digitalWrite(2,  (m7d[0] >> 0) & 1);
    digitalWrite(A5, (m7d[0] >> 1) & 1);
    digitalWrite(A4, (m7d[0] >> 2) & 1);
    digitalWrite(A3, (m7d[0] >> 3) & 1);
    digitalWrite(A2, (m7d[0] >> 4) & 1);
    digitalWrite(A1, (m7d[0] >> 5) & 1);
    digitalWrite(A0, (m7d[0] >> 6) & 1);
  }
  else if (a == 'y') {                            //Hiển thị chữ "y"
    digitalWrite(2,  (m7d[1] >> 0) & 1);
    digitalWrite(A5, (m7d[1] >> 1) & 1);
    digitalWrite(A4, (m7d[1] >> 2) & 1);
    digitalWrite(A3, (m7d[1] >> 3) & 1);
    digitalWrite(A2, (m7d[1] >> 4) & 1);
    digitalWrite(A1, (m7d[1] >> 5) & 1);
    digitalWrite(A0, (m7d[1] >> 6) & 1);
  }

  else {                                         //Tắt led
    digitalWrite(2, 1);
    digitalWrite(A5, 1);
    digitalWrite(A4, 1);
    digitalWrite(A3, 1);
    digitalWrite(A2, 1);
    digitalWrite(A1, 1);
    digitalWrite(A0, 1);
  }
}

void setup() {                      //Các chân tín hiệu led 7 đoạn
  pinMode(2, OUTPUT);
  pinMode(A5, OUTPUT);
  pinMode(A4, OUTPUT);
  pinMode(A3, OUTPUT);
  pinMode(A2, OUTPUT);
  pinMode(A1, OUTPUT);
  pinMode(A0, OUTPUT);
  write7d();

  pinMode(CD1, INPUT_PULLUP);
  pinMode(CD2, INPUT_PULLUP);
  pinMode(buzz, OUTPUT);


  myServo.attach(7);
  myServo.write(180);
  Serial.begin(9600);
  SPI.begin();
  rfid.PCD_Init();

}

void loop() {
  if (!digitalRead(CD1)) {                    // Nhấn CD1 và Đảo biến is_enable Nếu is_enable = true, hiển thị chữ "y", ngược lại hiển thị chữ "n"
    is_enable = !is_enable;
    if (is_enable)
    {
      write7d('y');
      digitalWrite(buzz, 1);
      delay(200);
      digitalWrite(buzz, 0);
    }
    else
    {
      write7d('n');
      digitalWrite(buzz, 1);
      delay(200);
      digitalWrite(buzz, 0);
    }


    while (!digitalRead(CD1));          //Chống dội nút CD1
  }
  else if (!digitalRead(CD2)) {         //Nhấn CD2 Nếu is_enable = false , Nếu is_open = false, mở cửa, đặt is_open = true Nếu ngược lại, đóng cửa, đặt is_open = false
    if (!is_enable)
    {

      if (!is_open) {
        is_open = true;
        myServo.write(90);
      }
      else {
        is_open = false;
        myServo.write(180);
      }
      while (!digitalRead(CD2));    //Chống dội nút CD2

    }


  }

  if (is_enable) readRC522();     //Nếu is_enable = true, đọc thẻ RFID
  if (is_enable && id[0] == 0x0B && id[1] == 0x70 && id[2] == 0x81 && id[3] == 0x22 && !is_open) {    
      is_open = true;
      id[0] = id[1] = id[2] = id[3] = 0;
      myServo.write(90);
      delay(1000);
  }
  else  if (is_enable && id[0] == 0x0B && id[1] == 0x70 && id[2] == 0x81 && id[3] == 0x22 && is_open) {
      is_open = false;
      id[0] = id[1] = id[2] = id[3] = 0;
      myServo.write(180);
      delay(1000);
  }
    else if (is_enable && id[0] == 0xe9 && id[1] == 0xb9 && id[2] == 0xbd && id[3] == 0x6e && !is_open) {
      is_open = true;
      id[0] = id[1] = id[2] = id[3] = 0;
      myServo.write(90);
      delay(500);
    }
     else if (is_enable && id[0] == 0xe9 && id[1] == 0xb9 && id[2] == 0xbd && id[3] == 0x6e && is_open) {
      is_open = false;
      id[0] = id[1] = id[2] = id[3] = 0;
      myServo.write(180);
      delay(500);
    }


else if (is_enable && !fakeCard(id)) {       //Nếu đọc mã thẻ bất kỳ và is_enable = true -> Buzzer kêu 3 lần -> Xóa bộ nhớ lưu mã thẻ
  digitalWrite(buzz, 1);
  delay(200);
  digitalWrite(buzz, 0);
  delay(100);
  digitalWrite(buzz, 1);
  delay(200);
  digitalWrite(buzz, 0);
  delay(100);
  digitalWrite(buzz, 1);
  delay(200);
  digitalWrite(buzz, 0);
  delay(100);
  id[0] = id[1] = id[2] = id[3] = 0;

}
}
void readRC522() {               //Đọc thẻ RFID và lưu trong bộ nhớ
  if (!rfid.PICC_IsNewCardPresent()) return;
  if (!rfid.PICC_ReadCardSerial()) return;
  id[0] = rfid.uid.uidByte[0];
  id[1] = rfid.uid.uidByte[1];
  id[2] = rfid.uid.uidByte[2];
  id[3] = rfid.uid.uidByte[3];s
}

bool fakeCard(uint8_t checkID[]) {             //Hàm kiểm tra thẻ không hợp lệ
  uint8_t true_count = 0;
  uint8_t Card[][4] = {{0, 0, 0, 0}, {0x0b, 0x70, 0x81, 0x22}, {0xe9, 0xb9, 0xbd, 0x6e}};      //Danh sách các thẻ hợp lệ
  for (auto &ICard : Card) {
    for (uint8_t i = 0; i < 4; i++)
    {

      if (checkID[i] == ICard[i]) true_count++;
    }
    if (true_count == 4) return true;
    else true_count = 0;
  }

  return false;

}
