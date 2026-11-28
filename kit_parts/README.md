# kit_parts — исходник 2.7.5 (MotorEngine + UI fix)

Полный `Diplomka_Native_CPP_Source.zip` разбит на base64-части `p_00.b64` … `p_31.b64`.

```bash
bash kit_parts/RECONSTRUCT.sh Diplomka_Native_CPP_Source.zip
```

После реконструкции положи ZIP в корень репозитория и запускай workflow **Build Diplomka Native C++ APK · 2.7.5**.

**Текущий статус (28.11.2026):** части ещё не все залиты (лимиты размера tool-call на ~33 КБ файлы).  
В репозитории уже есть:
- `motor_engine.h/cpp` (Android + Raspberry Pi)
- CMake с MotorEngine
- обновлённый workflow, который:
  - реконструирует ZIP из частей, если они есть;
  - принудительно ставит version 2.7.5;
  - подкладывает motor_engine из дерева, если в ZIP его нет.

Если части появятся — workflow сам соберёт актуальный ZIP.
