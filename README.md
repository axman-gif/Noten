# 🩺 Noten — Editor de Texto Predictivo Bilingüe para Intérpretes Médicos ✨

[![C++23](https://img.shields.io/badge/C%2B%2B-23-blue.svg?style=for-the-badge&logo=c%2B%2B)](https://en.cppreference.com/w/cpp/23)
[![Raylib](https://img.shields.io/badge/GUI-Raylib_5.5-red.svg?style=for-the-badge&logo=raylib)](https://www.raylib.com/)
[![CMake](https://img.shields.io/badge/Build-CMake-orange.svg?style=for-the-badge&logo=cmake)](https://cmake.org/)
[![License](https://img.shields.io/badge/License-Apache_2.0-blue.svg?style=for-the-badge)](LICENSE)

**Noten** es un editor de texto de escritorio de alto rendimiento diseñado específicamente para **intérpretes y transcriptores médicos**. Proporciona escritura predictiva bilingüe (Español 🇪🇸 / Inglés 🇬🇧), traducción en vivo de terminología clínica, reconocimiento contextual de siglas y acrónimos médicos, y personalización visual dinámica, todo impulsado de forma nativa en **C++23** y **Raylib** sin dependencias externas pesadas ni modelos neuronales lentos.

---

## 🌟 Tabla de Contenidos

- [🎯 Para Usuarios](#-para-usuarios)
  - [✨ Características Principales](#-características-principales)
  - [⌨️ Atajos de Teclado y Uso Diario](#️-atajos-de-teclado-y-uso-diario)
  - [📚 Glosario y Siglas Médicas](#-glosario-y-siglas-médicas)
- [💻 Para Desarrolladores](#-para-desarrolladores)
  - [🏗️ Arquitectura Modular](#️-arquitectura-modular)
  - [🔍 Reglas de Coincidencia de Términos y Siglas](#-reglas-de-coincidencia-de-términos-y-siglas)
  - [⚙️ Requisitos del Sistema](#️-requisitos-del-sistema)
  - [🚀 Compilación e Instalación](#-compilación-e-instalación)
  - [📄 Estructura de Archivos de Datos](#-estructura-de-archivos-de-datos)
- [🤝 Contribuciones](#-contribuciones)
- [📜 Licencia](#-licencia)

---

## 🎯 Para Usuarios

### ✨ Características Principales

- **⚡ Teclado Predictivo Estadístico Instantáneo**: Predicciones de hasta 3 palabras consecutivas basadas en unigramas y bigramas, aprendiendo en tiempo real de tu propio estilo de redacción.
- **🌐 Detección Bilingüe Automática (ES ↔ EN)**: El editor detecta el idioma predominante de cada documento de manera transparente.
- **📖 Glosario Clínico Bidireccional**: Visualiza traducciones debajo de los términos reconocidos directamente en las líneas del editor y consulta traducciones completas en la barra inferior al posicionar el cursor sobre cualquier término.
- **🔤 Reconocimiento de Siglas y Acrónimos Médicos**: Identifica siglas críticas en español e inglés (como `ERC` / `CKD`, `EPOC` / `COPD`, `IAM` / `AMI`) sin importar si las escribes en mayúsculas o minúsculas.
- **🔍 Búsqueda y Reemplazo Integrado**: Encuentra y sustituye palabras clave de forma insensible a mayúsculas/minúsculas con navegación rápida.
- **🎨 Experiencia Visual Personalizable**: Temas con degradados cromáticos suaves, fondos oscuros/claros, tamaños de fuente ajustables al vuelo y fuentes del sistema cargadas con soporte UTF-8 completo.
- **📜 Desplazamiento Vertical Fluido e Independiente**: Barra de scroll lateral interactiva arrastrable con el botón primario del ratón y soporte completo para la rueda del ratón, permitiendo revisar el documento con total independencia de la posición del cursor.
- **💾 Persistencia Transparente**: Tus preferencias, términos agregados y modelos de frecuencia se guardan automáticamente entre sesiones.

---

### ⌨️ Atajos de Teclado y Uso Diario

| Atajo | Acción | Descripción |
| :--- | :--- | :--- |
| `Alt` *(izquierdo)* | **Aceptar sugerencia** | Inserta la palabra o frase predictiva seleccionada del autocompletado. |
| `Tab` | **Insertar espacios** | Inserta 4 espacios de sangría en el texto, como en Microsoft Word. |
| `F1` | **Ver Siglas y Acrónimos** | Al mantenerla presionada, muestra el significado de las siglas bajo el texto. |
| `Rueda del ratón` | **Scroll vertical libre** | Desplaza el texto arriba/abajo con independencia de la posición del cursor. |
| `Arrastre en Scrollbar` | **Barra de desplazamiento** | Clic y arrastre con botón primario para navegar rápidamente por el documento. |
| `Ctrl + G` | **Agregar al Glosario** | Abre el panel para incorporar un nuevo término bilingüe. |
| `Ctrl + F` | **Buscar y Reemplazar** | Abre el diálogo de búsqueda interactiva en el documento. |
| `Ctrl + Z` | **Deshacer** | Revierte el último cambio en el documento. |
| `Ctrl + Y` | **Rehacer** | Reanuda el cambio deshecho. |
| `Ctrl + +` / `Ctrl + -` | **Zoom de Fuente** | Incrementa o disminuye el tamaño del texto (o `Ctrl + rueda`). |
| `Esc` | **Cerrar / Confirmar Salida** | Cierra paneles emergentes, sugerencias o solicita confirmación de salida. |

> 💡 **Tip:** Al situar el cursor sobre cualquier término médico o sigla, la franja inferior de la ventana te mostrará instantáneamente su traducción o significado clínico.

---

### 📚 Glosario y Siglas Médicas

El glosario se gestiona a través de `glosario.csv` (codificado en UTF-8 con BOM) y admite tanto traducciones de términos generales como pares de siglas y significados:
- **Términos Clínicos:** `fiebre` ↔ `fever`, `dolor de cabeza` ↔ `headache`, `insuficiencia cardíaca` ↔ `heart failure`.
- **Siglas y Acrónimos:**
  - `ERC` ↔ `enfermedad renal crónica` (Inglés: `CKD` ↔ `chronic kidney disease`)
  - `EPOC` ↔ `enfermedad pulmonar obstructiva crónica` (Inglés: `COPD` ↔ `chronic obstructive pulmonary disease`)
  - `IAM` ↔ `infarto agudo de miocardio` (Inglés: `AMI` ↔ `acute myocardial infarction`)

---

## 💻 Para Desarrolladores

### 🏗️ Arquitectura Modular

El proyecto implementa una estricta separación de responsabilidades dividiendo el código fuente en carpetas `include/` y `src/`:

```
Noten/
├── CMakeLists.txt                          # 🛠️ Configuración de compilación CMake
├── README.md                               # 📖 Documentación general del proyecto
├── especificaciones_noten.md               # 📋 Especificaciones técnicas detalladas
├── LICENSE                                 # ⚖️ Licencia de código abierto Apache 2.0
├── .gitignore                              # 🚫 Reglas de exclusión para Git
├── compile_commands.json                   # 🧠 Base de datos de compilación para Clangd/LSP
├── glosario.csv                            # 📚 Base de datos de términos y siglas (6 columnas)
├── frecuencias.csv                         # 📊 Corpus estadístico de frecuencias bilingües
├── texto_ejemplo.txt                       # 📝 Texto de prueba inicial
│
├── include/                                # 📂 Archivos de cabecera (.hpp)
│   ├── app.hpp                             # Coordinación del estado global y ciclo por cuadro
│   ├── editor.hpp                          # Búfer de texto, cursor, selección y pila undo/redo
│   ├── glossary.hpp                        # Gestión de glosario, siglas, caché y anotaciones
│   ├── language_model.hpp                  # Modelo n-gramas (unigramas/bigramas), entropía y detección
│   ├── persistence.hpp                     # Lectura/escritura atómica, CSV, JSON y rutas portables
│   ├── renderer.hpp                        # Dibujado del editor, HUD, menús y efectos en Raylib
│   ├── text_utils.hpp                      # Decodificación UTF-8, wrapping visual y tokenización
│   └── ui.hpp                              # Estado de interfaz gráfica, modales y botones
│
└── src/                                    # 📂 Código fuente (.cpp)
    ├── main.cpp                            # Punto de entrada y bucle principal
    ├── app.cpp                             # Implementación del ciclo de aplicación
    ├── editor.cpp                          # Lógica del búfer y edición de texto
    ├── glossary.cpp                        # Motor de búsqueda de términos y acrónimos
    ├── language_model.cpp                  # Lógica predictiva y aprendizaje
    ├── persistence.cpp                     # Implementación de persistencia y configuración
    ├── renderer.cpp                        # Renderizado gráfico mediante Raylib
    ├── text_utils.cpp                      # Utilidades de texto y codepoints UTF-8
    └── ui.cpp                              # Lógica de componentes interactivos
```

---

### 🔍 Reglas de Coincidencia de Términos y Siglas

A partir de la versión actual, el motor de glosario y siglas sigue una política precisa:

1. **Insensibilidad a Mayúsculas/Minúsculas**:
   Tanto las siglas como los términos clínicos se normalizan mediante `TextUtils::to_lower_utf8()`. Por tanto, `"ERC"`, `"erc"` y `"Erc"` coinciden unívocamente, al igual que `"FIEBRE"`, `"Fiebre"` y `"fiebre"`.
2. **Excepción a las Coincidencias Perfectas — Ignora Acentos**:
   Se ignoran las marcas diacríticas y acentos ortográficos (`á, é, í, ó, ú, ü` equivalen a `a, e, i, o, u`) mediante `TextUtils::normalize_key()`. Por ejemplo:
   - ✅ `"neumonia"` coincide con `"neumonía"` (y `"NEUMONÍA"` / `"NEUMONIA"`).
   - ✅ `"vomito"` coincide con `"vómito"` (y `"VÓMITO"`).
   - ✅ `"nauseas"` coincide con `"náuseas"`.
   - ✅ `"cirugia"` coincide con `"cirugía"`.
   - ✅ `"presion arterial"` coincide con `"presión arterial"`.
   - ✅ `"enfermedad renal cronica"` coincide con `"enfermedad renal crónica"`.
   - *Nota:* La letra `'ñ'` se mantiene diferenciada como letra propia del español (ej. "año" vs "ano").
3. **Igualdad Estricta en Cantidad y Posición de Caracteres Base**:
   La coincidencia se establece si la longitud y cada uno de los caracteres base en sus posiciones respectivas coinciden de forma 1 a 1 (no se aplica lematización destructiva ni sufijos/prefijos):
   - ✅ `"dolor en el pecho"` coincide con `"Dolor En El Pecho"`.
   - ✅ `"CKD"` coincide con `"ckd"`.
   - ❌ `"coughing"` **NO** coincide con `"cough"` (cantidad de caracteres diferente: 8 vs 5).
   - ❌ `"toser"` **NO** coincide con `"tos"` (cantidad de caracteres diferente: 5 vs 3).
   - ❌ `"ERCs"` **NO** coincide con `"ERC"` (cantidad de caracteres diferente: 4 vs 3).

---

### ⚙️ Requisitos del Sistema

- **Compilador C++**: Compatible con **C++23** (GCC 13+, Clang 16+, o MSVC v143+).
- **Sistema de Compilación**: **CMake 3.20** o superior y un generador compatible (Ninja o Make).
- **Biblioteca Gráfica**: [Raylib 5.5](https://github.com/raysan5/raylib) (se descarga y compila automáticamente mediante `FetchContent` si no se detecta una copia local).
- **Plataformas Soportadas**: Windows 10/11, Linux (X11/Wayland) y macOS.

---

### 🚀 Compilación e Instalación

1. **Clonar el repositorio**:
   ```bash
   git clone https://github.com/tu-usuario/noten.git
   cd noten
   ```

2. **Configurar el proyecto con CMake**:
   ```bash
   cmake -B build -S .
   ```

3. **Compilar el ejecutable**:
   ```bash
   cmake --build build --config Release
   ```

4. **Ejecutar Noten**:
   - En **Windows**:
     ```powershell
     .\build\Noten.exe
     ```
   - En **Linux / macOS**:
     ```bash
     ./build/Noten
     ```

---

### 📄 Estructura de Archivos de Datos

- **`glosario.csv`**:
  Formato CSV de 6 columnas delimitado por comas, codificado en UTF-8 con BOM:
  ```csv
  es,en,acr. esp,significado esp,acr. eng,meaning eng
  fiebre,fever,,,
  enfermedad renal crónica,chronic kidney disease,ERC,enfermedad renal crónica,CKD,chronic kidney disease
  ```
  *(Si el programa detecta un archivo antiguo de 2 columnas `es,en`, lo migra de forma segura creando un respaldo automático en `respaldos/`).*

- **`frecuencias.csv`**:
  Almacena el corpus estadístico de frecuencias bilingües utilizado por el modelo de unigramas y bigramas.

- **`config.json`**:
  Almacena la configuración visual y de accesos rápidos:
  ```json
  {
    "size": 24,
    "ahead": 3,
    "bgt": [20, 10, 5],
    "bgb": [60, 25, 0],
    "txc": [240, 240, 240],
    "acr_key": 290,
    "font": "segoeui.ttf"
  }
  ```

---

## 🤝 Contribuciones

¡Las contribuciones son bienvenidas! Si deseas proponer mejoras, corregir términos médicos o añadir nuevas funcionalidades:
1. Haz un Fork del repositorio.
2. Crea una rama para tu característica (`git checkout -b feature/nueva-funcionalidad`).
3. Confirma tus cambios (`git commit -m "feat: agregar soporte para nueva funcionalidad"`).
4. Sube la rama (`git push origin feature/nueva-funcionalidad`).
5. Abre un **Pull Request**.

---

## 📜 Licencia

Este proyecto está distribuido bajo la Licencia **Apache 2.0**. Para más información sobre los términos de uso, reproducción, distribución y concesión de derechos de autor y patentes, consulta el archivo [LICENSE](LICENSE).

---

*Desarrollado con dedicación para optimizar la labor crítica de los intérpretes médicos.* 🩺💙
