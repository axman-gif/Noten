Construye un editor de texto de escritorio con teclado predictivo bilingüe (español/inglés) para un intérprete médico, usando C++23 o superior y la biblioteca gráfica Raylib nativa para C/C++.

# 0. REGLAS DE EJECUCIÓN PARA AGENTES DE IDE

Estas instrucciones están dirigidas también a agentes de programación con acceso a un IDE, por ejemplo VS Code, Antigravity u otro entorno con herramientas de terminal y edición de archivos.

Cuando el agente tenga acceso a esas herramientas, NO debe limitarse a responder con código en el chat. Debe llevar a cabo la creación y configuración del proyecto en el workspace actual siguiendo este procedimiento:

1. Inspeccionar el workspace actual antes de crear archivos.
2. Detectar el sistema operativo, compilador de C++, CMake (si está disponible) y si Raylib ya está instalada/configurada.
3. Si falta alguna dependencia necesaria, solicitar explícitamente permiso al usuario para ejecutar los comandos de instalación/configuración correspondientes.
4. Antes de crear o modificar archivos existentes, solicitar permiso cuando el IDE/entorno lo requiera.
5. Antes de sobrescribir cualquier archivo existente, informar qué archivo será reemplazado y solicitar permiso para sobrescribirlo.
6. Antes de ejecutar comandos en la terminal que modifiquen el sistema, instalen paquetes, generen archivos, compilen o ejecuten el programa, solicitar los permisos necesarios que ofrezca el IDE.
7. No usar comandos destructivos, eliminar archivos, hacer `git reset --hard`, borrar directorios completos ni sobrescribir archivos existentes sin autorización explícita del entorno/usuario.
8. Una vez concedidos los permisos, crear el proyecto completo, escribir los archivos necesarios, configurar la compilación, compilar y ejecutar el programa para verificar que funciona.
9. Si la compilación produce errores, corregirlos en el workspace y volver a compilar, respetando de nuevo las políticas de permisos del IDE.
10. El objetivo final es dejar en el workspace un programa C++/Raylib funcional y ejecutable, no solamente un bloque de código.

Si el entorno no proporciona herramientas de archivos/terminal o no permite solicitar permisos, el modelo debe entregar los archivos y comandos necesarios claramente, sin fingir que los ejecutó.

# 1. ESTRUCTURA DEL PROYECTO Y TECNOLOGÍA

- Lenguaje: C++23 o superior.
- Biblioteca gráfica: Raylib nativa para C/C++. No usar la versión Python de Raylib, `pyray`, `ffi`, Python ni bindings de Python.
- El proyecto DEBE estar estructurado con una separación estricta entre cabeceras y código fuente:
  - Los archivos `.hpp` deben ubicarse dentro de la carpeta `include/` dentro de la carpeta del proyecto (`Noten/include`).
  - Los archivos `.cpp` deben ubicarse dentro de la carpeta `src/` dentro de la carpeta del proyecto (`Noten/src`).
- `src/main.cpp` debe contener únicamente la inicialización de la aplicación, la creación de los objetos principales y el bucle principal de alto nivel. No debe convertirse en un archivo monolítico.
- Estructura modular estándar del proyecto:
  - `src/main.cpp` — arranque, ventana Raylib y bucle principal.
  - `include/editor.hpp` / `src/editor.cpp` — estado del editor, cursor, selección, undo/redo, inserción y eliminación.
  - `include/language_model.hpp` / `src/language_model.cpp` — unigramas, bigramas, aprendizaje, entropía, detección de idioma y predicción.
  - `include/glossary.hpp` / `src/glossary.cpp` — carga/guardado del glosario y siglas, normalización insensible a mayúsculas/minúsculas y acentos (`normalize_key`), búsqueda de términos exactos en caracteres base, traducciones y anotaciones.
  - `include/renderer.hpp` / `src/renderer.cpp` — dibujo del editor, texto, cursor, selección, sugerencias, traducciones, barra superior y paneles.
  - `include/ui.hpp` / `src/ui.cpp` — interacción con botones, paneles, cuadros flotantes y menús.
  - `include/persistence.hpp` / `src/persistence.cpp` — CSV, JSON/configuración, rutas y persistencia.
  - `include/text_utils.hpp` / `src/text_utils.cpp` — UTF-8, tokenización, wrapping, `strip_accents`, `normalize_key`, búsqueda insensible a mayúsculas/minúsculas y funciones auxiliares de texto.
  - `include/app.hpp` / `src/app.cpp` — coordinación del estado global de la aplicación y del ciclo por cuadro, manteniendo `main.cpp` limpio y conciso.
  - `CMakeLists.txt` — configuración reproducible de compilación en la raíz del proyecto, configurando `target_include_directories(editor_predictivo PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include ${raylib_SOURCE_DIR}/src)`.
- El agente puede combinar algunos módulos estrechamente relacionados, pero NO debe convertir todo el programa en un único `.cpp`.
- Cada módulo debe tener una responsabilidad clara y evitar dependencias circulares.
- Las estructuras y funciones compartidas deben declararse en `.hpp` (dentro de `include/`) y definirse en `.cpp` (dentro de `src/`).
- Evitar variables globales innecesarias. Preferir clases, estructuras de estado y objetos de servicio que puedan pasarse explícitamente entre módulos.
- `src/main.cpp` idealmente debe quedar en torno a unas pocas decenas de líneas, encargándose principalmente de iniciar, ejecutar y cerrar la aplicación.
- Usar `CMakeLists.txt` para hacer reproducible la compilación. El agente debe crearlo o mantenerlo si no existe.
- No introducir frameworks adicionales para la interfaz. No usar Qt, SDL, Dear ImGui ni librerías de interfaz externas.
- Toda la interfaz visible al usuario y todos los comentarios del código deben estar en español.
- El programa debe ser de escritorio y funcionar con teclado y ratón.
- El proyecto debe compilar con un compilador C++ moderno compatible con C++23 o superior.

# 2. REGLAS DE CONFIGURACIÓN AUTOMÁTICA DEL PROYECTO

Cuando el agente tenga acceso a terminal y archivos:

- Buscar primero si el workspace ya contiene `src/main.cpp`, `CMakeLists.txt`, carpetas `include/` y `src/`, archivos de datos o una configuración previa de Raylib.
- Reutilizar configuraciones existentes compatibles cuando sea posible.
- Si Raylib no está disponible, determinar la forma más apropiada para el entorno actual (por ejemplo, instalación del sistema, gestor de paquetes o dependencia FetchContent de CMake) y pedir permiso antes de instalarla.
- Si se usa CMake, crear una configuración que compile los fuentes de `src/`, incluya las cabeceras de `include/`, encuentre o descargue Raylib y genere el ejecutable `editor_predictivo`.
- La configuración no debe depender de Python.
- Después de configurar, compilar y ejecutar una prueba del programa.
- Si ya existen archivos con el mismo nombre, no reemplazarlos silenciosamente: solicitar permiso para sobrescribirlos.
- Si el IDE ofrece permisos de "terminal", "edición de archivos", "instalación" o similares, pedirlos cuando sean necesarios en lugar de intentar eludirlos.

# 3. CONSTANTES Y ARCHIVOS

- `RAM_DIR = "R:/"` como ruta configurable de un RAM disk.
- `CSV_PATH` = `RAM_DIR/frecuencias.csv` si el directorio existe; si no, usar `frecuencias.csv` junto al ejecutable o en el directorio de trabajo del programa.
- `GLOS_PATH` = `glosario.csv` en el mismo directorio que `CSV_PATH`.
- `HERE` = directorio del ejecutable/programa o una ruta de datos definida de forma portable.
- `CFG_PATH` = `HERE/config.json`.
- `SAMPLE_PATH` = `HERE/texto_ejemplo.txt`.
- `H_MAX = 2.0` bits.
- `PROMOTE = 3`.
- `AHEAD = 3` global, con valores 1 a 3.
- `BAR = 48`.
- `WORD` debe representar palabras formadas por letras, ignorando dígitos y `_`, manteniendo compatibilidad con español e inglés.
- Ventana: 1000×650 px, redimensionable, título `"Editor predictivo ES/EN"`, 60 FPS.
- Desactivar la tecla de salida por defecto de Raylib mediante la API nativa equivalente a `SetExitKey(KEY_NULL)`.
- Esc se maneja manualmente según la sección de entrada.
- No usar funciones específicas de Python para rutas, regex, cachés o archivos; usar las equivalentes de la biblioteca estándar de C++.

# 4. FUENTE

- Buscar la primera fuente existente de:
  - `C:/Windows/Fonts/segoeui.ttf`
  - `C:/Windows/Fonts/arial.ttf`
  - `/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf`
  - `/System/Library/Fonts/Supplemental/Arial.ttf`
- Cargarla con `LoadFontEx` o la API nativa equivalente de Raylib, generando los codepoints necesarios.
- Incluir los codepoints ASCII 32–126 más:
  - `áéíóúüñÁÉÍÓÚÜÑ¿¡`
- El agente debe construir el arreglo de codepoints en C++ y pasarlo a Raylib junto con la cantidad de elementos.
- Mantener una caché de fuentes por tamaño para evitar recargar una misma fuente repetidamente.
- Si ninguna fuente existe, usar `GetFontDefault()`.
- Espaciado de letras: 1.

# 5. MODELO DE LENGUAJE (SIN REDES NEURONALES)

- Dos estructuras principales en memoria:
  - `uni[palabra] = [cuenta_es, cuenta_en]`
  - `bi[a][b] = [cuenta_es, cuenta_en]`
- Se pueden implementar mediante `std::unordered_map`, `std::map` u otra estructura estándar apropiada.
- Persistencia en `CSV_PATH`, UTF-8, cabecera `ngrama,es,en`.
- Una fila por unigrama (`palabra`) y por bigrama (`a b`).
- Cargar al inicio y guardar cada 25 palabras aprendidas y al cerrar.
- `entropy(valores)` = `−Σ p·log2(p)` con `p = v/suma`; usar 0 si la suma es 0.
- `close(a, b)`: verdadero si la distancia de edición es ≤ 1 (una sustitución, inserción o eliminación; sin transposiciones).
- `learn(w, prev, L, force=false)`:
  - convertir `w` a minúsculas;
  - si `w` no está en `uni` y no es `force`, buscar la primera palabra `u` de `uni` con `len(w) >= 4` y `close(w, u)`;
  - si existe, registrar `u` en lugar de `w`;
  - si no existe, incrementar `pending[w]`;
  - mientras `pending[w] < PROMOTE`, no registrar;
  - al llegar a `PROMOTE`, borrar de `pending` y registrar;
  - registrar como `uni[w][L] += 1`;
  - si existe `prev`, registrar `bi[prev][w][L] += 1`;
  - devolver la palabra aprendida, o `w` si quedó pendiente.
- `detect(texto)`:
  - tomar las últimas 12 palabras del texto;
  - sumar las cuentas es y en de las palabras presentes en `uni`;
  - devolver 1 (inglés) si `en > es`, si no 0 (español).
- `split(texto_hasta_cursor)` → `(head, pre, prev)`:
  - `pre` = letras finales en minúsculas;
  - `head` = lo anterior;
  - `prev` = última palabra de `head`;
  - `prev` es cadena vacía si `head` termina en `.`, `!`, `?`, salto de línea o espacios después de uno de ellos.
- `predict(pre, prev, L)`:
  - universo = `bi[prev]` si `pre` está vacío y existe `prev`; si no, `uni`;
  - seleccionar palabras que empiecen con `pre` y tengan cuenta[L] > 0;
  - puntaje = `cuenta[L] + 5 * bi[prev][w][L]` si existe ese bigrama;
  - ordenar de mayor a menor;
  - `H` = entropía de los 8 mejores puntajes;
  - para las 3 mejores:
    - iniciar secuencia `[w]`;
    - repetir `AHEAD - 1` veces:
      - obtener seguidores de `bi[ultimo]` con cuenta[L] > 0;
      - si no hay seguidores o su entropía es mayor que `H_MAX`, detenerse;
      - en otro caso agregar el seguidor con mayor cuenta;
  - devolver como máximo 3 secuencias y `H`.
- Semilla, solo si `uni` está vacío al iniciar, aprendida con `force=true` palabra por palabra encadenando `prev`:
  - ES: `"doctor me duele la cabeza desde ayer"`
  - ES: `"insuficiencia cardíaca congestiva con dolor en el pecho"`
  - ES: `"el paciente presenta dolor abdominal y fiebre"`
  - EN: `"the patient has chest pain and shortness of breath"`
  - EN: `"congestive heart failure with leg swelling"`
  - EN: `"the patient reports headache since yesterday"`

# 6. GLOSARIO MÉDICO Y SIGLAS ES↔EN

- `glosario.csv` con UTF-8 con BOM y encabezado de 6 columnas:
  `es,en,acr. esp,significado esp,acr. eng,meaning eng`
- Si existe un archivo previo de 2 columnas (`es,en`), se migra automáticamente a 6 columnas preservando los datos y generando una copia de respaldo en `respaldos/glosario_YYYYMMDD_HHMMSS.csv`.
- Semillas iniciales del glosario y siglas:
  - dolor de cabeza=headache
  - dolor en el pecho=chest pain
  - dolor abdominal=abdominal pain
  - fiebre=fever
  - náuseas=nausea
  - vómito=vomiting
  - mareo=dizziness
  - falta de aire=shortness of breath
  - tos=cough
  - presión arterial=blood pressure
  - hipertensión=hypertension
  - diabetes=diabetes
  - infarto de miocardio=myocardial infarction
  - accidente cerebrovascular=stroke
  - insuficiencia cardíaca=heart failure
  - insuficiencia cardíaca=cardiac failure
  - insuficiencia cardíaca congestiva=congestive heart failure
  - neumonía=pneumonia
  - asma=asthma
  - alergia=allergy
  - fractura=fracture
  - hinchazón=swelling
  - sangrado=bleeding
  - análisis de sangre=blood test
  - radiografía=x-ray
  - ayuno=fasting
  - cirugía=surgery
  - anestesia=anesthesia
  - receta=prescription
  - alta médica=discharge
  - urgencias=emergency room
  - consentimiento informado=informed consent
  - escalofríos=chills
  - fatiga=fatigue
  - erupción=rash
  - convulsión=seizure
  - desmayo=fainting
  - estreñimiento=constipation
  - diarrea=diarrhea
  - embarazo=pregnancy
  - Siglas iniciales:
    - `ERC` (enfermedad renal crónica) / `CKD` (chronic kidney disease)
    - `EPOC` (enfermedad pulmonar obstructiva crónica) / `COPD` (chronic obstructive pulmonary disease)
    - `IAM` (infarto agudo de miocardio) / `AMI` (acute myocardial infarction)
- **Regla estricta de detección y normalización (con excepción de acentos)**:
  - Al detectar tanto siglas como términos del glosario se deben ignorar las diferencias entre mayúsculas y minúsculas (`to_lower_utf8`) **y se deben ignorar los acentos diacríticos / tildes** (`strip_accents`: `á/é/í/ó/ú/ü` coinciden con `a/e/i/o/u`), mediante `normalize_key()`.
  - La coincidencia entre los caracteres base en cantidad y posición debe ser igual (coincidencia 1 a 1 de caracteres base sin sufijos, prefijos ni lematizaciones que alteren la cantidad de caracteres).
  - **Excepción a las coincidencias perfectas**:
    - **Ignora acentos**: Las vocales con acento agudo (`á, é, í, ó, ú`), diéresis (`ü`), acento grave o circunflejo son tratadas como equivalentes a sus vocales base sin acento (`a, e, i, o, u`), tanto en mayúsculas como en minúsculas.
    - La consonante `'ñ'` se preserva como letra propia del alfabeto para evitar ambigüedades diagnósticas y clínicas (ej. "ano" vs "año").
  - Ejemplos de coincidencia válida:
    - `"fiebre"`, `"Fiebre"`, `"FIEBRE"` coinciden con `"fiebre"`.
    - `"dolor de cabeza"`, `"Dolor de cabeza"`, `"DOLOR DE CABEZA"` coinciden con `"dolor de cabeza"`.
    - `"neumonia"`, `"neumonía"`, `"NEUMONÍA"` y `"NEUMONIA"` coinciden con `"neumonía"`.
    - `"vomito"`, `"vómito"`, `"VOMITO"` y `"VÓMITO"` coinciden con `"vómito"`.
    - `"nauseas"` y `"náuseas"` coinciden con `"náuseas"`.
    - `"cirugia"` y `"cirugía"` coinciden con `"cirugía"`.
    - `"presion arterial"` y `"presión arterial"` coinciden con `"presión arterial"`.
    - `"enfermedad renal cronica"` y `"enfermedad renal crónica"` coinciden con `"enfermedad renal crónica"`.
    - `"ERC"`, `"erc"`, `"Erc"` coinciden con la sigla `"ERC"`.
    - `"COPD"`, `"copd"`, `"Copd"` coinciden con la sigla `"COPD"`.
  - Ejemplos que NO coinciden (longitud o caracteres base distintos):
    - `"coughing"` NO coincide con `"cough"` (cantidad de caracteres diferente: 8 vs 5; no se aplica lematización destructiva).
    - `"toser"` NO coincide con `"tos"` (cantidad de caracteres diferente: 5 vs 3).
    - `"headaches"` NO coincide con `"headache"` (cantidad diferente: 9 vs 8).
    - `"ERCs"` NO coincide con `"ERC"` (cantidad diferente: 4 vs 3).
- Estructuras en memoria:
  - `g_es[normalize_key(es)] = [traducciones en]`
  - `g_en[normalize_key(en)] = [traducciones es]`
  - `acr_es[normalize_key(sigla)] = [significados es]`
  - `acr_en[normalize_key(sigla)] = [significados en]`
  - Sin duplicados en las listas de traducciones y significados.
- `gl_add(es, en)` agrega a ambos diccionarios (usando claves `normalize_key`) y enseña al modelo de lenguaje (`uni`), con `force=true`, las palabras del término que aún no existan.
- `acr_add(diccionario, sigla, significado)` registra la sigla normalizada (`normalize_key`) y su significado.
- `gloss_save(es, en)` llama a `gl_add` y anexa la fila con 6 columnas en `glosario.csv`.
- `term_at(texto, i)`:
  - Primero verifica si la posición `i` coincide con una sigla (`tokenize_acronyms`) registrada en `acr_es` o `acr_en` de forma insensible a mayúsculas y acentos (`normalize_key`).
  - Luego localiza el token de palabra que contiene `i` (`tokenize_words`);
  - Prueba frases de 3, luego 2, luego 1 palabra que contengan a dicho token;
  - Exige que los tokens consecutivos estén separados por exactamente un espacio;
  - Convierte la frase mediante `normalize_key`;
  - Si coincide en `g_es`, devuelve `(texto mostrado original, traducciones, "EN")`;
  - Si no, si coincide en `g_en`, devuelve `(texto mostrado original, traducciones, "ES")`;
  - En caso de palabra única, verifica también los diccionarios de acrónimos;
  - En otro caso, `None`.

# 7. ANOTACIONES EN EL TEXTO Y TECLA DE SIGLAS/ACRÓNIMOS

- `word_lookup(clave, L)`:
  - Coincidencia exacta de la palabra normalizada (`normalize_key`) en `g_es` o `g_en` según el idioma detectado `L`.
  - Respeta la regla de igualdad exacta en cantidad y posición de caracteres base (sin lematización por sufijos ni prefijos), ignorando mayúsculas y acentos.
- `annotate(texto, lm)`:
  - Detecta idioma con `lm.detect(texto)`.
  - Tokeniza palabras.
  - En cada posición prueba primero frases de 3 y 2 palabras exactas con `normalize_key`, luego la palabra sola con `word_lookup`.
  - Si hay coincidencia y la primera traducción normalizada es distinta de la clave (`normalize_key(traducción) != normalize_key(término)`), genera una anotación `(inicio_byte, fin_byte, etiqueta)`.
  - Omite términos idénticos en ambos idiomas, como `"diabetes"`.
  - Etiqueta = hasta 2 traducciones unidas por `" / "`.
  - Avanza `n` palabras cuando se reconoce una frase.
  - Memoriza las anotaciones calculadas y solo las recalcula cuando cambia el texto o el glosario.
- `annotate_acr(texto, L)`:
  - Tokeniza acrónimos y siglas (`tokenize_acronyms`), permitiendo letras, dígitos y guiones `-` o barras `/`.
  - Normaliza la sigla (`normalize_key`) y busca en `acr_es` y `acr_en` con coincidencia exacta de cantidad y posición de caracteres base ignorando acentos y mayúsculas.
  - Devuelve anotaciones con el significado de la sigla.
- Tecla de siglas / acrónimos:
  - Configurable en `config.json` mediante `acr_key` (código de tecla Raylib, por defecto 290 = `KEY_F1`).
  - Mientras la tecla `F1` esté presionada (`ui.acr_down`), el renderizador dibuja `annotate_acr(texto, L)` en lugar de las anotaciones del glosario general.
  - Al soltarla, vuelven a mostrarse las anotaciones estándar de `annotate(texto, lm)`.
- Dibujo de anotaciones bajo las palabras:
  - Para cada línea visible, las anotaciones cuyo inicio cae en la línea se dibujan debajo del término.
  - `x = 24 + ancho del texto de la línea hasta el inicio del término`.
  - `y = y de la línea + tamaño + 2`.
  - Fuente con tamaño `asz` y color de texto con opacidad/alfa 170.
- La franja inferior muestra en todo momento las traducciones o significado de la sigla/término bajo el cursor (`term_at`).

# 8. ESTADO DEL EDITOR

Globales:
- `text` (cadena UTF-8);
- `cur` (índice del cursor);
- `anc` (ancla de selección o valor nulo);
- `undo` y `redo` como vectores de estados `(text, cur)`;
- `sel` como opción elegida del menú.

Selección:
- `(min, max)` entre `cur` y `anc` si `anc` no es nulo y distinto de `cur`.

Funciones:
- `push()` agrega `(text, cur)` a `undo` y vacía `redo`.
- `ins(s)` inserta `s` en `cur`.
- `delete_sel()` hace `push`, borra el rango, deja `cur` en el inicio, pone `anc` en nulo y devuelve si había selección.
- `move(n, shift)`:
  - si `shift` y no hay ancla, `anc = cur`;
  - si no `shift`, `anc = null`;
  - `cur = n`, limitado a `[0, text.length()]`.
- `finalize()` aprende, mediante `learn`, la palabra parcial que termina en el cursor.
- `accept(opción)`:
  - `push`;
  - aprender cada palabra con `force=true`;
  - reemplazar la palabra parcial por las palabras unidas con espacio más un espacio final;
  - si ya hay un espacio inmediatamente después del cursor, consumirlo;
  - mover el cursor tras lo insertado;
  - `anc = null`.
- `find_next(q)`:
  - buscar sin distinguir mayúsculas/minúsculas;
  - iniciar desde el final de la selección o desde el cursor;
  - hacer vuelta al inicio al llegar al final;
  - seleccionar la coincidencia.
- `replace_cur(q, r)`:
  - si la selección coincide con `q` ignorando mayúsculas/minúsculas, hacer `push` y reemplazarla.

# 9. DISEÑO Y TEXTO

- Margen izquierdo: 24 px.
- Texto comienza en `y = BAR + 16`.
- Tamaño de letra inicial: 24.
- Rango de tamaño: 12–64.
- Paso de tamaño: 2.
- `asz = max(10, tamaño / 2)`.
- Alto de línea = `tamaño + asz + 8`.
- Reservar siempre la fila adicional bajo cada línea para traducciones.
- Dibujar solo las líneas visibles.
- `wrap(texto, fuente, tamaño, ancho_máximo = ancho_ventana − 48)` devuelve `(índice_inicio, texto_línea)`:
  - dividir por `"\n"`;
  - cada salto ocupa 1 índice;
  - cada párrafo se parte con la lógica equivalente a `\s*\S+\s*|\s+`;
  - iniciar una nueva línea si `(línea + token).rstrip()` supera el ancho máximo y la línea no está vacía.
- `lineof(líneas, idx)` = última línea con inicio ≤ idx.
- `line_end(líneas, i)` = fin del contenido de la línea; si la línea fue partida por ajuste y la siguiente empieza donde termina esta, usar la longitud sin espacios finales.
- `x_to_idx(líneas, i, x, …)` debe encontrar el índice de la línea cuya posición horizontal se aproxime más a `x`, empezando desde x=24 y limitándose a `line_end`.
- `pos_at(mx, my, …)` convierte un clic en índice.
- Cursor:
  - barra de 2 px;
  - alto igual al tamaño de letra;
  - color de letra.
- Scroll vertical y barra de desplazamiento:
  - Rueda del ratón = `3 × alto_de_línea` por paso, desplazando el contenido verticalmente de forma suave dentro de `[0, max_scroll]`.
  - El desplazamiento opera de forma completamente libre e independiente de la posición del cursor de texto: la rueda del ratón y el arrastre de la barra de scroll permiten revisar cualquier parte del documento sin que la vista se bloquee o salte hacia donde esté el cursor.
  - Solo cuando el usuario interactúa activamente mediante el teclado (escritura de caracteres, Enter, Backspace, Delete, Tab, flechas de dirección, Home/End, etc.) y no se está arrastrando la barra de desplazamiento, el visor se ajusta automáticamente para traer el cursor a la zona visible.
  - Margen de visibilidad: 1 línea abajo (más espacio para franja inferior si existe) y `BAR + 8` arriba.
  - Límite máximo de scroll: `max_scroll = max(0, total_h + 32 + 2 * line_h - view_h)`.
  - Barra de desplazamiento lateral (scrollbar):
    - Ubicada en el lateral derecho: pista redondeada de 10 px de ancho en `x = ancho - 13`, `y = BAR + 4`, con alto que ocupa toda la zona visible hasta el borde inferior (o sobre la franja de traducción si está presente).
    - Pista oscura translúcida `(25, 25, 25, 140)`.
    - Tirador redondeado (thumb) con altura proporcional al contenido: `alto = max(28, (alto_visible / (alto_visible + max_scroll)) * alto_pista)`.
    - Colores: naranja brillante `(200, 110, 20, 240)` al arrastrar, gris claro `(150, 150, 150, 220)` al pasar el cursor (hover), y gris `(90, 90, 90, 180)` en reposo.
    - Permite clic y arrastre con el botón primario del ratón en tiempo real. Un clic en la pista centra y desplaza el tirador suavemente hacia la posición pulsada.
    - La interacción con la barra de scroll no mueve el cursor de texto ni altera la selección activa.
- Selección:
  - rectángulo por línea;
  - alto = alto de línea;
  - color opuesto al de la letra: `(255 − R, 255 − G, 255 − B)`;
  - si el salto de línea al final está seleccionado, agregar 8 px;
  - el texto seleccionado sigue dibujándose en el color de letra.

# 10. ENTRADA

## Ratón

En zona de texto (`y > BAR` y fuera de paneles flotantes y barra de scroll):

- clic coloca cursor;
- Shift+clic fija ancla en el cursor si no había y mueve el cursor al clic;
- doble clic (`< 0,35 s` y `≤ 1` carácter de diferencia respecto al clic anterior) selecciona la secuencia de letras alrededor;
- arrastrar con botón izquierdo selecciona texto (con auto-scroll si el puntero se desplaza por encima o por debajo del área visible);
- al soltar, si `anc == cur`, quitar la selección.
- la rueda del ratón desplaza el texto hacia arriba/abajo independientemente del cursor de texto;
- clic y arrastre con el botón primario sobre la barra de desplazamiento lateral navega por el documento en tiempo real.

## Escritura

Ignorar escritura de texto cuando Ctrl esté activo.

- Letra:
  - si no existe palabra parcial, hacer `push()` antes;
  - insertar.
- Carácter imprimible no alfabético:
  - `finalize()`;
  - `push()` si no se borró una selección;
  - insertar.
- Enter:
  - borrar selección o hacer `push`;
  - `finalize`;
  - insertar `"\n"`.
- Retroceso con repetición:
  - borrar selección;
  - con Ctrl, borrar la palabra anterior usando la lógica equivalente a `\S*\s*$` sobre el texto antes del cursor, haciendo `push`;
  - en otro caso, borrar un carácter.
- Supr:
  - borrar selección o carácter siguiente.
- Tab con repetición:
  - no acepta sugerencias de autocompletado;
  - borra selección si existe o hace `push`;
  - `finalize`;
  - inserta 4 espacios (`"    "`) como sangría (estilo Microsoft Word).
- Ctrl+A selecciona todo.
- Ctrl+C y Ctrl+X usan el portapapeles del sistema; X además borra.
- Ctrl+V pega, elimina `\r` y reemplaza la selección.
- Ctrl+Z deshace y guarda el estado actual en `redo`.
- Ctrl+Y rehace.
- Ctrl+F abre buscar/reemplazar; la selección actual precarga el campo de búsqueda.
- Ctrl+G abre `"agregar al glosario"`; la selección precarga el campo del idioma detectado.
- Ctrl+1/2/3 fijan `AHEAD`.
- Ctrl + '+' y Ctrl + '-' aumentan o reducen el tamaño de fuente (paso de 2 px, rango de 12 a 64 px). También soportado mediante Ctrl + rueda del ratón en la zona del editor.
- Deshacer no revierte lo aprendido por el modelo.

## Movimiento

- ←/→ con repetición.
- Sin Shift y con selección, colapsar al borde correspondiente de la selección.
- Home/End = inicio de la línea visual / `line_end`.
- Con Shift, extender selección.
- ↑/↓:
  - si el menú de sugerencias está activo y no se pulsa Shift, cambiar la opción elegida circularmente;
  - en cualquier otro caso mover el cursor una línea visual conservando la columna horizontal en píxeles mediante una variable de "x objetivo";
  - recalcular la x objetivo si el cursor cambió por otro medio;
  - en la primera línea ↑ va al inicio del texto;
  - en la última línea ↓ va al final;
  - con Shift extender la selección.
- Alt (izquierdo, `KEY_LEFT_ALT`) acepta la opción elegida del menú de sugerencias predictivas.

## Esc

Esc actúa por capas, una por pulsación:

1. cerrar el cuadro de buscar/glosario si está abierto;
2. si no, cerrar el panel de colores;
3. si no, ocultar el menú de sugerencias si estaba activo;
4. si no queda nada abierto:
   - si el texto está vacío, cerrar el programa;
   - si hay texto, mostrar un aviso centrado:
     - ancho = texto más ancho + 40 px;
     - alto = 90 px;
     - fondo `(20,20,20,245)`;
     - borde gris;
     - `"¿Cerrar el editor?"`
     - `"El texto no se guarda.  Esc = cerrar  |  otra tecla = cancelar"`
     - fuente 20.
5. con el aviso visible:
   - otro Esc cierra;
   - cualquier otra tecla cancela.
6. al cerrar:
   - guardar `frecuencias.csv`;
   - guardar `config.json`.
7. cerrar con la X de la ventana no pide confirmación.

# 11. MENÚ DE SUGERENCIAS

- Recalcular solo cuando cambia `(text, cur)`.
- `L = detect(texto hasta cursor)`.
- `sel = 0`.
- No mostrar opciones si el carácter inmediatamente después del cursor es una letra.
- Cada opción muestra sus palabras unidas por espacios.
- El menú está activo cuando el texto cambió.
- Se vuelve inactivo con cualquier movimiento del cursor:
  - clic;
  - flechas;
  - Home;
  - End;
  - o Esc.
- Dibujo:
  - debajo del cursor;
  - `x = min(x_cursor, ancho − ancho_menú − 8)`;
  - `y = y_cursor + alto_de_línea + 4`;
  - máximo 3 filas;
  - alto de fila = `tamaño + 14`;
  - ancho = texto más ancho + 90;
  - fondo `(25,12,4,245)`;
  - fila elegida `(120,48,0)`;
  - parte ya escrita en blanco;
  - resto en `(255,140,40)`;
  - a la derecha, entropía en gris `(170,170,170)` con tamaño 12:
    `"H=x.x bits"`.
- No dibujar mientras haya un cuadro de buscar/glosario abierto.
- Como el alto de línea incluye la fila de traducción, el menú queda siempre debajo de la traducción.

# 12. BUSCAR/REEMPLAZAR Y GLOSARIO

- Cuadro flotante de 392×104 px en `(ancho − 400, BAR + 8)`.
- Si el panel de colores está abierto, desplazarlo a la izquierda `PW + 12` px.
- Dos campos:
  - fondo `(60,60,60)`;
  - campo activo `(90,70,30)`;
  - etiquetas con fuente 18:
    - `"Buscar:"` / `"Reemplazar:"`
    - o `"Español:"` / `"Inglés:"`
  - campo en `x+110`;
  - ancho 272;
  - alto 26;
  - filas en `y+8` y `y+40`;
  - pista en tamaño 12 en `y+78`.
- Con el cuadro abierto, el teclado va solo a los campos:
  - texto imprimible se agrega;
  - Retroceso borra;
  - Tab cambia de campo;
  - Esc cierra.
- Enter:
  - en buscar = siguiente coincidencia;
  - en reemplazo = reemplaza la actual y salta a la siguiente;
  - Ctrl+Enter = reemplaza todas ignorando mayúsculas/minúsculas.
- En modo glosario:
  - Enter guarda el par si ambos campos tienen texto y cierra.
- Pistas:
  - `"N coincidencias | Enter: sig. | Tab: campo | Ctrl+Enter: todo | Esc"`
  - `"Enter: guardar en glosario | Tab: campo | Esc"`
- Traducción bajo el cursor:
  - si no hay cuadro abierto y `term_at(text, cur)` devuelve algo;
  - dibujar una franja inferior de 34 px;
  - color `(15,15,15,235)`;
  - texto:
    `término  ->  trad1, trad2   [EN|ES]`
  - fuente 18;
  - color `(255,200,120)`;
  - `x = 14`.

# 13. BARRA SUPERIOR

Altura 48 px, color `(15,15,15)`.

Elementos con fuente por defecto tamaño 20, en y=17.

Clic izquierdo = avanzar.
Clic derecho = retroceder.

- Botones gris `(60,60,60)`:
  - `"A-"` en `(12,10,36,28)`;
  - `"A+"` en `(54,10,36,28)`;
  - etiqueta `"{tamaño}px"` en x=98.
- `"Fondo:"` en x=155 (dejando espacio libre tras el indicador de tamaño de letra para garantizar que nunca se solapen).
- Recuadro de degradado en `(225,10,36,28)` que cicla `GRADS`.
- `"Letra:"` en x=275.
- Recuadro en `(338,10,28,28)` que cicla `TXT`.
- Ambos con borde gris.
- `"Palabras:"` en x=380.
- Tres botones de 34×28 en x=`475+40*(k-1)`, con números 1, 2 y 3.
- Activo `(200,110,20)`.
- Inactivos `(60,60,60)`.
- Fijan `AHEAD` y fuerzan recalcular sugerencias.
- Botón `"Colores"` en `(596,10,100,28)`.
  - activo `(200,110,20)`;
  - inactivo `(60,60,60)`;
  - clic izquierdo abre/cierra panel de colores.
- Botón `"Fuente"` en `(706,10,150,28)`.
  - activo `(200,110,20)`;
  - inactivo `(60,60,60)`;
  - clic izquierdo abre/cierra menú de fuentes.
- La sección de idioma en pantalla no se muestra (la detección de idioma opera en segundo plano para el modelo predictivo y glosario sin ocupar espacio visual en la barra superior).
- `GRADS` =
  - `((20,10,5),(60,25,0))`
  - `((10,30,60),(20,60,80))`
  - `((10,10,10),(40,40,40))`
  - `((40,20,70),(120,48,0))`
  - `((245,240,230),(210,200,185))`
  - `((30,80,50),(10,30,20))`
- `TXT` =
  - `(240,240,240)`
  - `(20,20,20)`
  - `(255,200,120)`
  - `(150,255,170)`
  - `(255,130,130)`
  - `(130,200,255)`
- Estado de color:
  - `bgt` = fondo arriba;
  - `bgb` = fondo abajo;
  - `txc` = letra.
- El área de texto usa un degradado vertical de `bgt` a `bgb` bajo la barra.

# 14. PANEL "COLORES"

- Panel a la derecha.
- `PW = 372`.
- `PY = BAR + 8`.
- `PREV = 120`.
- `PH = PREV + 20 + 3·110 + 8 = 478`.
- x = `ancho − PW − 12`.
- fondo `(20,20,20,250)`;
- borde gris.

Vista previa:
- degradado `bgt → bgb` en `(x+10, PY+10, PW−20, 120)`;
- texto de ejemplo ajustado a ancho `PW−44`;
- fuente 16;
- interlineado 22;
- color de letra;
- inicio `(x+20, PY+18)`;
- recortado mediante scissor mode de Raylib.
- Leer `texto_ejemplo.txt` cada vez que se abre el panel.
- Si no existe, crearlo con:
  `"Paciente femenina de 54 años con dolor opresivo en el pecho desde hace dos días, falta de aire al subir escaleras, náuseas y sudor frío. Niega fiebre."`

Tres secciones:
- `"Fondo (arriba)"`
- `"Fondo (abajo)"`
- `"Letra"`

Etiqueta tamaño 14 en `(x+12, PY+PREV+21+110k)`.

Paleta:
- 36 colores;
- cuadrícula de 12 columnas × 3 filas;
- cada recuadro 24×24 px;
- x=`x+10+(i%12)·28`;
- y=`PY+PREV+20+110k+18+(i//12)·28`.
- clic izquierdo fija `bgt`, `bgb` o `txc`;
- color actual con borde blanco;
- demás con borde gris `(90,90,90)`.
- Los clics dentro del panel no mueven el cursor del texto.

`PALETTE`, en orden:
- `(0,0,0),(25,25,25),(45,45,45),(70,70,70),(15,25,60),(10,45,90),(0,60,60),(10,60,30),(50,40,10),(80,25,0),(70,10,20),(45,15,70)`
- `(130,130,130),(220,40,40),(240,120,20),(245,200,30),(130,200,40),(30,170,80),(20,180,170),(40,140,230),(60,80,220),(130,70,220),(220,60,170),(150,100,60)`
- `(255,255,255),(235,235,235),(210,210,210),(255,190,190),(255,215,170),(255,245,170),(210,245,180),(180,240,215),(180,225,255),(200,205,255),(235,205,255),(245,230,210)`

# 15. PERSISTENCIA DE PREFERENCIAS

Al cerrar:
- guardar `config.json` en UTF-8 con:
  - `size`
  - `ahead`
  - `bgt` (arreglo `[r, g, b]`)
  - `bgb` (arreglo `[r, g, b]`)
  - `txc` (arreglo `[r, g, b]`)
  - `acr_key`
  - `font` (opcional si hay fuente cargada)

Al iniciar:
- leer `config.json`;
- `size` limitado a 12–64;
- componentes de color limitados a 0–255;
- `ahead` limitado a 1–3;
- `acr_key` (código de tecla Raylib para alternar vista de acrónimos, por defecto 290 = `KEY_F1`);
- si falta o es inválido:
  - tamaño = 24;
  - `GRADS[0]`;
  - `TXT[0]`;
  - `ahead = 3`;
  - `acr_key = 290`.
- Si `config.json` contiene el valor anterior 96 (`KEY_GRAVE`), se migra automáticamente a 290 (`KEY_F1`).

No introducir una dependencia JSON externa si puede evitarse. Preferir una serialización sencilla compatible con el formato indicado implementada en `persistence.cpp`.

# 16. ORDEN DE ARRANQUE Y CIERRE

Orden de arranque:

1. cargar `frecuencias.csv`;
2. sembrar el modelo si `uni` está vacío;
3. cargar/crear glosario;
4. crear ventana Raylib;
5. leer configuración;
6. entrar al bucle principal.

Por cuadro, mantener este orden lógico:

En actualización:
1. botones de la barra superior;
2. menú de fuentes si está desplegado;
3. selección de colores en el panel lateral;
4. interacción y arrastre con la barra de desplazamiento lateral (scrollbar);
5. ratón en el área de texto y rueda de desplazamiento;
6. atajos y teclado del editor o del cuadro de buscar/glosario;
7. recálculo de sugerencias cuando cambie el estado correspondiente;
8. ajuste de scroll al cursor (únicamente si hubo interacción activa de teclado y no se está arrastrando la barra).

En renderizado:
1. fondo degradado y área del editor (texto, selección, cursor, anotaciones);
2. barra superior;
3. menú de sugerencias predictivas;
4. franja inferior de traducción bajo el cursor;
5. barra de desplazamiento vertical (scrollbar);
6. paneles flotantes (colores, buscar/glosario, fuentes);
7. aviso toast y confirmación de cierre.

Al salir:

1. guardar `frecuencias.csv`;
2. guardar `config.json`;
3. cerrar las fuentes/cachés necesarias;
4. cerrar la ventana y liberar recursos de Raylib.

# 17. REQUISITOS DE CALIDAD Y COMPATIBILIDAD

- Mantener exactamente la lógica funcional descrita arriba; no simplificar características.
- No sustituir el modelo estadístico por una red neuronal.
- No cambiar el objetivo bilingüe ES/EN.
- No eliminar funciones de edición, selección, deshacer/rehacer, búsqueda/reemplazo, glosario, traducciones, sugerencias, colores o persistencia.
- No depender de Python.
- Usar API nativa de Raylib para C/C++.
- Mantener UTF-8 para el contenido almacenado y para las cadenas de interfaz.
- Tener especial cuidado con el manejo de caracteres Unicode de español.
- El programa debe compilar sin warnings críticos y ejecutarse de forma estable.
- El agente debe comprobar que el binario se genere correctamente y, cuando el entorno permita ejecutar aplicaciones gráficas, abrir el programa para una validación básica.
- Si el entorno no puede ejecutar una ventana gráfica, al menos compilar y verificar el binario.

# 18. CRITERIO DE ENTREGA

Cuando estas instrucciones se ejecuten dentro de un IDE con herramientas de agente:

- crear o actualizar el proyecto directamente en el workspace;
- solicitar todos los permisos necesarios para terminal, instalación de dependencias, creación/modificación de archivos y sobrescritura;
- no sobrescribir silenciosamente archivos existentes;
- crear la estructura modular de `.cpp` y `.hpp`;
- configurar el sistema de compilación;
- compilar;
- corregir errores de compilación;
- dejar listo el programa ejecutable.

La aplicación debe quedar implementada como un proyecto modular de C++ con cabeceras en `include/` y fuentes en `src/`.

El punto de entrada debe ser:

`src/main.cpp`

El resto de la implementación debe repartirse entre los módulos definidos en la sección 1 (`include/` y `src/`), manteniendo separadas las responsabilidades.

El proyecto debe poder abrirse, compilarse y ejecutarse desde el IDE.

# 19. RESULTADO ESPERADO

El resultado final debe ser un proyecto C++/Raylib modular, mantenible y compilable, con la estructura de carpetas `include/` y `src/`, cuyo punto de entrada sea `src/main.cpp`.

El proyecto debe implementar un editor predictivo bilingüe ES/EN para un intérprete médico con:

- teclado predictivo estadístico;
- predicción de hasta 3 palabras;
- selección y edición de texto;
- búsqueda y reemplazo;
- glosario médico bidireccional;
- detección automática ES/EN;
- traducciones visibles en el texto;
- panel de colores;
- tamaños de fuente configurables;
- persistencia del aprendizaje y preferencias;
- interfaz Raylib;
- compilación nativa C++;
- sin Python.

No entregar únicamente una explicación conceptual: en un entorno con capacidades de agente, realizar las operaciones necesarias para crear y configurar el proyecto y verificarlo.

