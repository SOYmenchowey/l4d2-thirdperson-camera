# L4D2 Third Person Camera

Proyecto independiente para Left 4 Dead 2 (Windows x86). La DLL solo incluye el overlay y los controles de cámara de tercera persona: activación, hotkey y distancia.

## Incluye

- Activar/desactivar tercera persona desde el panel o una hotkey asignable.
- Slider de distancia (30–600).
- Panel compacto que se abre y cierra con `Insert`.
- Guardado local de los tres ajustes de tercera persona.

No contiene el ajuste del viewmodel, mira/crosshair ni otras páginas del overlay.

## Requisitos y compilación

- Windows, Visual Studio 2022 Build Tools o Visual Studio con MSVC x86 y Windows SDK.
- Left 4 Dead 2 para Windows (proceso de 32 bits) para usar la DLL dentro del juego.

Ejecuta `build.bat` desde Developer Command Prompt x86 de Visual Studio. El script también busca MSVC con `vswhere` si `cl.exe` no está en el `PATH`. Genera `build\l4d2_thirdperson.dll` y `l4d2_thirdperson.dll` en la raíz.

## Uso

1. Inicia Left 4 Dead 2.
2. Inyecta `build\l4d2_thirdperson.dll` con un inyector compatible.
3. Pulsa `Insert` para abrir el panel.
4. Activa la cámara, ajusta la distancia y asigna una tecla con el botón de hotkey. `Esc` cancela la captura.
5. Pulsa **Guardar** para conservar el ajuste.

La configuración se guarda en `vmconfig.ini` junto a la DLL cargada. `vmconfig.example.ini` es una plantilla limpia y no contiene preferencias personales.

## Cómo funciona

- `src/thirdperson/thirdperson.cpp` valida el nombre de `z_view_distance` antes de escribir los valores de tercera persona.
- `src/thirdperson/camera_transition.h` comprueba firmas de la build del juego y sincroniza la transición sin desplazar modos de cámara nativos.
- `src/overlay/dx9_hook.cpp` dibuja el panel mediante `Present` de Direct3D 9 y recupera los recursos tras `Reset`.
- `src/overlay/menu.cpp` maneja la hotkey de tercera persona, el menú y el archivo de configuración, guardado de forma atómica.

## Offsets y compatibilidad

Los offsets son de 32 bits y específicos de una build del juego. Se publican para que puedan revisarse y actualizarse:

| Archivo | Módulo | Offset/RVA |
| --- | --- | --- |
| `src/thirdperson/thirdperson.cpp` | `client.dll` | `RVA_Z_VIEW_DISTANCE = 0x7C1588`; nombre `+0x0C`, float `+0x2C`, entero `+0x30`. |
| `src/thirdperson/camera_transition.h` | `client.dll` | `0x726BD8`, `0x634FB`, `0x26B3F0`, `0x256908`, `0x7C15A4`, `0x2630`. |
| `src/overlay/dx9_hook.cpp` | `engine.dll` | `0x4268EC` y ángulos `+0x4AAC`, usados para mantener la vista mientras se maneja el panel. |

Las validaciones pueden desactivar una ruta si la build no coincide, pero no garantizan compatibilidad con futuras versiones. Revalida los offsets tras actualizaciones de Steam. No hay una identificación de build del juego asociada a estos valores.

## Dependencias y licencias

- Este proyecto: MIT, ver `LICENSE.txt`.
- Dear ImGui: MIT, ver `third_party/imgui/LICENSE.txt`.
- Fuente Inter incluida: SIL Open Font License 1.1, ver `third_party/imgui/OFL-1.1.txt`.
