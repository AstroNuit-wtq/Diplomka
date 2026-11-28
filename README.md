# Diplomka 2.7.5 — Android + Raspberry Pi 4B

Единый репозиторий дипломного проекта. Репозиторий `ile_cradle` архивирован — вся разработка ведётся здесь.

## Версия 2.7.5

- **MotorEngine (VESC UART)** подключён в Android и Raspberry Pi.
- Кнопка **STOP** и сигнал препятствия (`vision.stopSignal`) вызывают `applyStopFlag`:
  - duty / current = 0 → машинка останавливается;
  - драйверы (Flipsky Mini Dual FSESC / 75100) **остаются под питанием и в состоянии arm**.
- Исправлен **оверлей кнопок** в развёрнутой панели инструментов Android (`ImGui::NewLine` между рядами).
- Маршрутизация по-прежнему **«ближайший сосед»** на обеих платформах:
  1. сначала соседи текущего waypoint из графа `route.csv`;
  2. при отсутствии — fallback на ближайшую непосещённую точку.

## Структура

- `app/` — Android NativeActivity, Camera2, ImGui, MotorEngine, classical vision.
- `raspberry_pi/` — отдельная сборка для Pi 4B (CSI, SDL2, OpenGL ES) + тот же MotorEngine и nearest-neighbor.
- `docs/` — архитектура, статус, changelog.
- `tests/` — переносимые C++ тесты.
- `.github/workflows/build-astro-cpp.yml` — сборка APK arm64-v8a.

## Сборка Android (GitHub Actions)

Положи полный исходный ZIP `Diplomka_Native_CPP_Source.zip` в корень (или держи дерево `app/`) и запусти workflow **Build Diplomka Native C++ APK**.

## Сборка Raspberry Pi

```bash
cd raspberry_pi
BUILD_JOBS=2 bash build_pi.sh
bash run_pi.sh
```

## Драйверы

Поддерживаются VESC-совместимые ESC (Flipsky Mini Dual FSESC4.20, Flipsky 75100 и аналоги).
По умолчанию backend = Null (логирование). Для железа укажи UART-устройство, например `/dev/ttyUSB0` или `/dev/ttyACM0`, dualChannel=true для одной платы Dual.

## Лицензии сторонних компонентов

См. `THIRD_PARTY_NOTICES.md`.
