#include <MsTimer2.h>            // タイマー割り込みを利用する為に必要なヘッダファイル
#include <stdio.h>
#define AD0 54 //センサのピンで指定する奴じゃない本当のアドレス？あとで書き直す
#define AD1 55 //0から順にデータ配列に入る、足の番号1~3とデータ配列1～3に入るセンサーデータはそろえる
#define AD2 56　//今はA0と本来のA0はそろっている。
#define AD3 57
#define AD4 58
#define AD5 59
#define AD6 60
#define AD7 61
//54A0、なし、55A1制圧タンク、56A2足1-弁2と3、57A3足2弁5と6,58A4足3弁7と8、59A5負圧タンク向け
int current = 0;
int count = 0; //データログ用のカウンタ
int sensorNum = 5; //使ってるセンサの数　配列に使う
int pin = AD0; // 初期のADCチャンネル

// グローバル変数の定義
int currentPressure = 0; // 現在の空気圧
int exhaustValveTime = 0; // 排気バルブ制御時間
int supplyValveTime = 0;  // 吸気バルブ制御時間
int rooptime = 25 ; //制御タイミング

int EXITleg[10] = {0}; // 弁のピン番号を格納する配列
int inleg[10] = {0}; //上に同じ

int nontauchpuls = 10; //不感帯の幅　たぶんアナログリードだから要計算
int airroomNun =3 ; //空気室の数

int date[2][10] = {{0}};  // 全ての要素を0で初期化
 //データバッファ領域
int currentTime = 0;

enum OperatingMode {
  PC_COMMAND_MODE,
  TIME_WALK_MODE,
  STANDBY_MODE
};

OperatingMode currentMode = TIME_WALK_MODE; // デフォルトは待機モード

unsigned long timeWalkStartTime = 0;
int timeWalkTargetIndex = 0;

int targetPressure[3] = {0, 0, 0};　//目標値の初期配列

void setup() {
  // セットアップ
  // センサーの初期化、バルブの初期化、通信の初期化などを行う
  // タイマー割り込みを設定する
  MsTimer2::set(rooptime, flash);     // 25ms毎にflash( )割込み関数を呼び出す様に設定
  EXITleg[1] = 2; // EXITleg1にピン番号2を割り当てる（例）
  inleg[1] = 3;
  EXITleg[2] = 6; 
  inleg[2] = 5;
  EXITleg[3] = 8; 
  inleg[3] = 7;
  // 他のピン番号の設定も同様に行う
  // ここはセットアップ。今回使うのはアナログピン1~3
  // 圧力計ごとにアナログピンを使う
  Serial.begin(38400);
  pinMode( EXITleg[1], OUTPUT );
  pinMode( inleg[1], OUTPUT );
  pinMode( EXITleg[2], OUTPUT );
  pinMode( inleg[2], OUTPUT );
  pinMode( EXITleg[3], OUTPUT );
  pinMode( inleg[3], OUTPUT );
  //pinMode( invuc1, OUTPUT );
  delay(2000);
  MsTimer2::start();             // タイマー割り込み開始
}

void loop() {
  // その他の処理
  if (Serial.available() > 0) {
    char command = Serial.read();
    switch (command) {
      case 'P':
        currentMode = PC_COMMAND_MODE; //基本的に秒数歩行でやりますよと
        break;
      case 'T':
        currentMode = TIME_WALK_MODE;
        timeWalkStartTime = 0; // 秒数歩行モードの開始時刻を記録
        break;
      case 'S':
        currentMode = STANDBY_MODE;
        break;
      default:
        break;
    }
  }

  switch (currentMode) {
    case PC_COMMAND_MODE:
      if (Serial.available() >= 3) {
        for (int i = 0; i < 3; i++) {
          targetPressure[i] = Serial.parseInt();//例えば50-60-70にセットするときは50 60 70と送信
        }
        // シリアルモニターに確認のための出力
        for (int i = 0; i < 3; i++) {
          Serial.print("Target Pressure for Room ");
          Serial.print(char('A' + i));
          Serial.print(": ");
          Serial.println(targetPressure[i]);
        }
      }
      break;
    case TIME_WALK_MODE:
      if (currentTime - timeWalkStartTime < 15000) {
        // 5秒ごとにtargetPressureを任意セットに切り替える
        if (currentTime*25 < 5000) {
          targetPressure[0] = 60 ;
          targetPressure[1] = 60 ;
          targetPressure[2] = 60 ;
        } else if (currentTime * 25 < 10000) {
          targetPressure[0] = 90 ;
          targetPressure[1] = 90 ;
          targetPressure[2] = 90 ;
        } else {
          targetPressure[0] = 20 ;
          targetPressure[1] = 50 ;
          targetPressure[2] = 80 ;
        }
      } else {
        currentTime = 0; // タイマーリセット
      }
      break;
    case STANDBY_MODE:
      // 待機モードではtargetPressureを0に設定する
      for (int i = 0; i < 3; i++) {
        targetPressure[i] = 0;
      }
      break;
    default:
      break;
  }
}


void flash() {
  for (int i = 0; i < sensorNum; i++)
  {
   get_current_data();
  }

  for (int i = 0; i < airroomNun; i++){

   controlAirPressure(i); // 空気圧制御を25msごとに実行 
  }
  currentTime = currentTime + 1;
  datelog();
}

//標準的には、
//アクチュエータ制御するスレッドor割込み処理
//センサ値を読むスレッドor割り込みルーチン
//もし AD が終わってたら、AD値を current に入れて（過去データ溜めてる配列にも入れる？）、AD開始する

void get_current_data() {
  current = analogRead(pin);
  next_channel();
  current_to_bafa();
}

void next_channel() {
  switch (pin) {
    case AD0: pin = AD1; break;
    case AD1: pin = AD2; break;
    case AD2: pin = AD3; break;
    case AD3: pin = AD4; break;
    case AD4: pin = AD0; break;
    //case AD5: pin = AD0; break;
    default: pin = AD0; break;
  }
}

void current_to_bafa() {
  date[0][count] = current;
  if(count == sensorNum-1) {
    count = 0;
  }
  else{
    count = count+1;
  }
}

//データを格納する、2*センサ数の配列≒(0~1、センサ数-1)
void datelog() {
    for (int j = 0; j < sensorNum; j++) {

      Serial.print(date[1][j]);
      Serial.print("\t");
      date[1][j] = date[0][j];
        
    }
    Serial.println( );              // 改行の出力
}


// 吸気バルブの制御
void openSupplyValve(int targetlegnun) {
  // 吸気バルブを開く処理
  digitalWrite(inleg[targetlegnun], HIGH);
}

void closeSupplyValve(int targetlegnun) {
  // 吸気バルブを閉じる処理を記述する
  digitalWrite(inleg[targetlegnun], LOW);
}

// 排気バルブの制御
void openExhaustValve(int targetlegnun) {
  // 排気バルブを開く処理を
  digitalWrite(EXITleg[targetlegnun], HIGH);
}

void closeExhaustValve(int targetlegnun) {
  // 排気バルブを閉じる処理を記述する
  digitalWrite(EXITleg[targetlegnun], LOW);
}


void controlAirPressure(int i) {
  // 空気圧制御アルゴリズムを実装
  int currentPressure = date[0][i+1];
  int pressureDifference = targetPressure[i] - currentPressure;

  if (pressureDifference > nontauchpuls) {
    // 目標値よりも低い場合の制御
    openSupplyValve(i); // 吸気バルブを開く
    closeExhaustValve(i); // 排気バルブを閉じる
  } else if (pressureDifference < -nontauchpuls) {
    // 目標値よりも高い場合の制御
    closeSupplyValve(i); // 吸気バルブを閉じる
    openExhaustValve(i); // 排気バルブを開く
  } else {
    // 目標値に近い場合の制御
    closeSupplyValve(i); // 吸気バルブを閉じる
    closeExhaustValve(i); // 排気バルブを閉じる
  }
}


//メインが、目標値(target)を更新する
//圧力制御の方法
//案１）不感帯を設けて、排気弁・吸気弁を開け閉めする。
//案３）圧力誤差（現在圧と目標圧の差）に応じた時間だけ吸気or排気する

//void control() {
  // 現在の空気圧と目標の空気圧を比較
  //int pressureDifference = targetPressure - currentPressure;
  //if (pressureDifference > 0) {
    //supplyValveTime = pressureDifference * K1; // 吸気バルブを開ける時間
    //closeExhaustValve(); // 排気バルブを閉じる
    //openSupplyValve(supplyValveTime); // 吸気バルブを開ける
  //} else if (pressureDifference < 0) {
    //exhaustValveTime = abs(pressureDifference) * K2; // 排気バルブを開ける時間
    //closeSupplyValve(); // 吸気バルブを閉じる
    //openExhaustValve(exhaustValveTime); // 排気バルブを開ける
  //} else {
    //closeExhaustValve(); // 両方のバルブを閉じる
    //closeSupplyValve();
  //}
//}