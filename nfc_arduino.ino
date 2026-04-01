#include <SPI.h>
#include <MFRC522.h>
#include <Keyboard.h>
#include <avr/wdt.h>

#define SS_PIN 10
#define RST_PIN 9
MFRC522 mfrc522(SS_PIN, RST_PIN);

// --- 授权卡片列表 ---
const byte AUTH_CARDS[][4] = {
  {0x66, 0x9B, 0x60, 0xB8},
  {0xC9, 0xF1, 0x65, 0x11},
  {0x50, 0x0C, 0xF2, 0x55}
};
const int AUTH_COUNT = sizeof(AUTH_CARDS) / 4;

// --- 加密配置 ---
// 这里的密文是你的密码与卡片 Block 4 原始数据 XOR 后的结果
const char ENCRYPTED_PASSWORD[] = "\x5F\x4B\x5C\x57\x40\x54"; 
const byte KEY_BLOCK_ADDR = 4;
MFRC522::MIFARE_Key MIFARE_SECTOR_KEY;

unsigned long lastActionTime = 0;
const long COOLDOWN = 1500; // 1.5秒冷却，防止连续触发

void xor_crypt(char *data, size_t data_len, const char *key, size_t key_len) {
  for (size_t i = 0; i < data_len; i++) {
    data[i] = data[i] ^ key[i % key_len];
  }
}

void setup() {
  // Serial.begin(9600); // 调试时取消注释
  SPI.begin();
  mfrc522.PCD_Init();
  Keyboard.begin();
  
  // 初始化默认密钥 A (0xFF 0xFF 0xFF 0xFF 0xFF 0xFF)
  for (byte i = 0; i < 6; i++) MIFARE_SECTOR_KEY.keyByte[i] = 0xFF;

  wdt_enable(WDTO_4S); 
}

void loop() {
  wdt_reset(); 

  // 1. 检测并选择卡片
  if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) return;

  // 2. 冷却检查
  unsigned long currentTime = millis();
  if (currentTime - lastActionTime < COOLDOWN) {
    mfrc522.PICC_HaltA();
    return;
  }

  // 3. UID 身份匹配
  bool isAuthorized = false;
  for (int i = 0; i < AUTH_COUNT; i++) {
    if (memcmp(mfrc522.uid.uidByte, AUTH_CARDS[i], 4) == 0) {
      isAuthorized = true;
      break;
    }
  }

  if (isAuthorized) {
    // 4. 尝试读取扇区数据（增加重试机制）
    byte readBuffer[18];
    byte bufferSize = 18;
    bool readSuccess = false;

    // 验证扇区
    if (mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, KEY_BLOCK_ADDR, &MIFARE_SECTOR_KEY, &(mfrc522.uid)) == MFRC522::STATUS_OK) {
      // 最多尝试读取 3 次
      for(int retry = 0; retry < 3; retry++) {
        if (mfrc522.MIFARE_Read(KEY_BLOCK_ADDR, readBuffer, &bufferSize) == MFRC522::STATUS_OK) {
          readSuccess = true;
          break;
        }
        delay(50); 
      }
    }

    if (readSuccess) {
      // --- 执行锁定/解锁动作 ---
      
      // A. 执行锁屏
      Keyboard.press(KEY_LEFT_GUI);
      Keyboard.press('l');
      delay(120);
      Keyboard.releaseAll();

      // B. 等待 Windows 切换动画（如果这里太短，后续输入会丢失）
      delay(1800); 

      // C. 唤醒并输入
      Keyboard.write(' '); // 唤醒屏幕
      delay(400);

      // D. 解密密码
      char pwd[sizeof(ENCRYPTED_PASSWORD)];
      memcpy(pwd, ENCRYPTED_PASSWORD, sizeof(ENCRYPTED_PASSWORD));
      xor_crypt(pwd, sizeof(pwd) - 1, (char*)readBuffer, 16);

      // E. 模拟键盘输入
      Keyboard.print(pwd);
      
      // 立即清除内存明文
      memset(pwd, 0, sizeof(pwd));
      
      delay(100);
      Keyboard.press(KEY_RETURN);
      delay(50);
      Keyboard.releaseAll();

      lastActionTime = currentTime;
    }
  }

  // 停止本轮通信，释放射频场
  mfrc522.PCD_StopCrypto1();
  mfrc522.PICC_HaltA();
}