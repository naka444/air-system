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
