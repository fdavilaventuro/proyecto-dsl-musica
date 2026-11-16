# Music DSL Compiler

## 1. Estado del Proyecto

Este repositorio contiene un compilador funcional para un DSL (lenguaje específico
de dominio) musical implementado en C++.

El compilador utiliza:

- Un lexer manual.
- Un parser descendente recursivo (*Recursive Descent Parser*).
- Un Árbol Sintáctico Abstracto (AST).
- El patrón Visitor para separar las fases del compilador.
- Un visitante de comprobación semántica.
- Un visitante de optimización.
- Un visitante de generación de código.

En lugar de generar ensamblador nativo, el backend transpila el AST a Python.
El programa Python generado utiliza `winsound.Beep` para reproducir las notas en
Windows.

El proyecto también incluye un IDE interactivo en Tkinter con:

- Editor de archivos `.music`.
- Visualización del AST original y optimizado.
- Visualización del Python generado.
- Consola de errores.
- Botones para compilar y ejecutar la música.

### Organización del repositorio

```text
proyecto-dsl-musica/
├── src/
│   ├── ast.h
│   ├── parser.h
│   ├── parser.cpp
│   ├── type_checker.h
│   ├── type_checker.cpp
│   ├── optimizer.h
│   ├── optimizer.cpp
│   ├── gencode_visitor.h
│   ├── gencode_visitor.cpp
│   ├── print_ast.h
│   ├── print_ast.cpp
│   └── main.cpp
├── tools/
│   └── ide.py
├── build/
│   └── music_compiler.exe
├── examples/
│   └── sarias_song.music
├── generated/
│   ├── <archivos Python generados>
│   └── .last_opened
├── .gitignore
└── README.md
```

`build/` y `generated/` contienen artefactos locales y están excluidos del
control de versiones mediante `.gitignore`.

## 2. Requisitos previos

Se necesita instalar:

1. **Un compilador de C++17**, por ejemplo:
   - `g++` de MinGW/MSYS2 en Windows.
   - `g++` de GCC en Linux.
2. **Python 3**.
3. **Tkinter**, normalmente incluido en las instalaciones estándar de Python.
4. **Windows**, si se desea reproducir el audio generado, porque el backend usa
   `winsound`.

Verificación rápida:

```powershell
g++ --version
python --version
python -c "import tkinter; print('Tkinter disponible')"
```

## 3. Instrucciones de Compilación y Ejecución

### Compilar el compilador C++

Desde la raíz del proyecto, en PowerShell:

```powershell
New-Item -ItemType Directory -Force build | Out-Null

g++ -std=c++17 -Wall -Wextra -Wpedantic -Werror `
  -Isrc `
  src\main.cpp `
  src\parser.cpp `
  src\type_checker.cpp `
  src\optimizer.cpp `
  src\gencode_visitor.cpp `
  src\print_ast.cpp `
  -o build\music_compiler.exe
```

En una terminal compatible con Bash:

```bash
mkdir -p build
g++ -std=c++17 -Wall -Wextra -Wpedantic -Werror \
  -Isrc \
  src/main.cpp \
  src/parser.cpp \
  src/type_checker.cpp \
  src/optimizer.cpp \
  src/gencode_visitor.cpp \
  src/print_ast.cpp \
  -o build/music_compiler
```

### Ejecutar el compilador desde la terminal

Usa un archivo `.music` de `examples/` o de cualquier otra ubicación, y
ejecuta:

```powershell
.\build\music_compiler.exe examples\sarias_song.music --print-ast --optimize
```

El compilador:

1. Lee el archivo `.music`.
2. Construye el AST.
3. Ejecuta el `TypeCheckerVisitor`.
4. Imprime el AST original si se solicita.
5. Aplica optimizaciones si se usa `--optimize`.
6. Genera el AST optimizado si se solicita.
7. Escribe un archivo `.py` con el mismo nombre base dentro de `generated/`.

Al compilar `examples\sarias_song.music`, el resultado se guarda en:

```text
generated\sarias_song.py
```

### Opciones del CLI

```text
music_compiler <archivo.music> [--print-ast] [--optimize] [--print-opt-ast]
```

| Opción | Descripción |
|---|---|
| `--print-ast` | Imprime el AST antes de optimizar. |
| `--optimize` | Ejecuta el `OptimizerVisitor`. |
| `--print-opt-ast` | Imprime el AST optimizado y activa la optimización. |

### Ejecutar el IDE

Desde la raíz del proyecto:

```powershell
python tools\ide.py
```

El IDE busca el compilador en este orden:

1. `build\music_compiler.exe`
2. `build\music_compiler`
3. `build\musicc.exe`
4. `build\musicc`
5. El `PATH` del sistema.

Al pulsar **Abrir Archivo**, el IDE muestra un selector filtrado a archivos
`*.music`, carga el archivo elegido en el editor y guarda su ruta absoluta en
`generated/.last_opened`. Al iniciar de nuevo, intenta restaurar ese archivo
si todavía existe. Si es la primera ejecución o el archivo fue eliminado,
carga `examples/sarias_song.music`.

Los botones **Guardar** (`Ctrl+S`) y **Guardar como** (`Ctrl+Shift+S`) permiten
persistir el contenido editado. **Guardar como** inicia el diálogo en
`examples/`, usa la extensión `.music` y actualiza `generated/.last_opened`.
El archivo activo se muestra en el título de la ventana y en la etiqueta de
entrada del editor.

Al pulsar **Compilar**, el IDE:

1. Guarda el contenido del editor en el archivo musical activo.
2. Ejecuta el compilador con `--print-ast` y muestra el resultado en
   **AST Original**.
3. Si está marcado **Aplicar Optimizaciones**, ejecuta una segunda fase con
   `--optimize --print-opt-ast` y muestra el resultado en **AST Optimizado**.
4. Si las optimizaciones están desactivadas, **AST Optimizado** indica que
   coincide con el AST original.
5. Muestra el código Python final en **Código Python Generado**.
6. Muestra errores y códigos de salida en **Consola de Errores**.

El checkbox **Aplicar Optimizaciones** está activado por defecto y permite
comparar el resultado optimizado con el resultado sin optimizaciones.

Al pulsar **Ejecutar**, el IDE ejecuta:

```text
python generated/<nombre_de_la_cancion>.py
```

## 4. Tutorial paso a paso

### Paso 1: Crear una canción

Crea un archivo con extensión `.music`, por ejemplo
`examples\mi_cancion.music`:

```text
// Una melodía corta
tempo 120;

play C4, 1;
play E4, 1;
play G4, 2;
rest 1;

loop 2 {
    play G4, 0.5;
    play A4, 0.5;
    rest 0.5;
}
```

Cada instrucción termina con `;`. El bloque de `loop` usa llaves y puede
contener cualquier otra instrucción, incluso otro `loop`.

### Paso 2: Compilar desde la terminal

Después de compilar el compilador C++, ejecuta desde la raíz del proyecto:

```powershell
.\build\music_compiler.exe examples\mi_cancion.music --print-ast --optimize --print-opt-ast
```

El comando analiza el archivo, valida su contenido, imprime el AST original,
aplica las optimizaciones, imprime el AST optimizado y genera:

```text
generated\mi_cancion.py
```

La carpeta `generated/` se crea automáticamente y está ignorada por Git. No
es necesario crearla ni añadir sus archivos al repositorio.

### Paso 3: Ejecutar la canción

En Windows, reproduce el Python generado con:

```powershell
python generated\mi_cancion.py
```

El programa utiliza `winsound.Beep`; por eso la reproducción de audio está
orientada a Windows.

### Paso 4: Entender las duraciones musicales

Las duraciones de `play` y `rest` se expresan en **pulsaciones o beats**, no
en segundos. Con `tempo 120`, una pulsación dura medio segundo porque:

```text
milisegundos = duración * 60000 / BPM
```

En este DSL, `1` beat equivale a una **negra**. Por tanto, las duraciones
habituales son:

| Valor DSL | Figura musical | Equivalencia | Duración a 120 BPM |
|---:|---|---:|---:|
| `0.25` | semicorchea | un cuarto de negra | 125 ms |
| `0.5` | corchea | la mitad de una negra | 250 ms |
| `1` o `1.0` | negra | un beat | 500 ms |
| `1.5` | negra con puntillo | una negra más media negra | 750 ms |
| `2` o `2.0` | blanca | dos beats | 1000 ms |
| `3` | blanca con puntillo | tres beats | 1500 ms |
| `4` o `4.0` | redonda | cuatro beats | 2000 ms |

La misma escala se aplica a los silencios. Por ejemplo:

```text
play C4, 0.25; // C4 dura una semicorchea
play D4, 0.5;  // D4 dura una corchea
play E4, 1;    // E4 dura una negra
rest 2;        // silencio de una blanca
rest 4;        // silencio de una redonda
```

Si se cambia el tempo, cambia el tiempo real pero no la relación entre las
figuras. A `tempo 60`, una negra dura 1000 ms; a `tempo 240`, dura 250 ms.
Las duraciones deben ser estrictamente mayores que cero.

### Paso 5: Escribir notas

Una nota se escribe con una letra de `A` a `G`, una octava y, opcionalmente,
un sostenido (`#`) o un bemol (`b`):

```text
play C4, 1;
play F#4, 0.5;
play Bb3, 0.5;
```

El comprobador acepta las 88 teclas del piano clásico, desde `A0` hasta
`C8`, incluyendo equivalencias como `C#4` y `Db4`. No se debe separar la
alteración de la nota:

```text
play C #4, 1; // Incorrecto
```

### Paso 6: Usar silencios y repeticiones

Un silencio se escribe con `rest` y una duración:

```text
rest 0.5;
```

Un loop repite todas sus instrucciones:

```text
loop 2 {
    play C4, 0.5;
    rest 0.25;
}
```

El número de repeticiones debe ser entero y no negativo. `loop 0` no produce
audio y se elimina cuando se compila con `--optimize`.

### Paso 7: Trabajar con el IDE

Inicia el IDE desde la raíz:

```powershell
python tools\ide.py
```

El flujo recomendado es:

1. Pulsa **Abrir Archivo** y selecciona un archivo `.music`, o comienza con
   `examples\sarias_song.music`.
2. Edita el contenido en el panel izquierdo.
3. Usa **Guardar** o `Ctrl+S` para sobrescribir el archivo activo.
4. Usa **Guardar como** o `Ctrl+Shift+S` para crear una nueva canción.
5. Pulsa **Compilar**. El IDE muestra el AST original, el AST optimizado, el
   Python generado y los errores en pestañas separadas.
6. Desmarca **Aplicar Optimizaciones** si quieres comparar el resultado sin
   optimizar.
7. Pulsa **Ejecutar** para reproducir el archivo Python generado.

El IDE guarda la ruta absoluta del último archivo abierto en
`generated/.last_opened`. Esa ruta es local a cada computadora y está
ignorada por Git.

### Paso 8: Diagnosticar errores

Si el compilador informa un error, revisa la línea y columna mostradas. Los
casos más comunes son:

- Falta de `;` al final de una instrucción.
- Llaves sin cerrar en un `loop`.
- Nota fuera del rango `A0`–`C8`.
- Duración `0` o negativa.
- Número de repeticiones negativo o no entero.
- Uso de una nota separada, como `C #4`.

Corrige el archivo y vuelve a compilar. El compilador devuelve código `0` si
termina correctamente y código `1` si encuentra un error.

## 5. Guía de Uso (Referencia del Lenguaje)

### Gramática

Program   ::= Statement*
Statement ::= TempoStmt | PlayStmt | RestStmt | LoopStmt
TempoStmt ::= "tempo" NUMBER ";"
PlayStmt  ::= "play" ID "," NUMBER ";"
RestStmt  ::= "rest" NUMBER ";"
LoopStmt  ::= "loop" NUMBER "{" Statement* "}"

### Sintaxis rápida

| Instrucción | Forma | Función |
|---|---|---|
| `tempo` | `tempo NUMBER;` | Establece el BPM para las instrucciones siguientes. |
| `play` | `play ID, NUMBER;` | Reproduce una nota durante la duración indicada. |
| `rest` | `rest NUMBER;` | Inserta un silencio durante la duración indicada. |
| `loop` | `loop NUMBER { ... }` | Repite un bloque de instrucciones. |

### Reglas léxicas y semánticas

- Se permiten espacios, tabulaciones y saltos de línea entre tokens.
- Los comentarios comienzan con `//` y terminan al final de la línea.
- Una nota usa una letra de `A` a `G`, una octava y opcionalmente `#` o `b`.
- Solo se aceptan las 88 teclas desde `A0` hasta `C8`, incluyendo equivalencias
  como `C#4` y `Db4`.
- Las duraciones de `play` y `rest` deben ser mayores que cero. La tabla de
  figuras musicales está en el tutorial.
- El tempo predeterminado es `120 BPM`; los valores explícitos deben ser
  positivos.
- El número de repeticiones de un `loop` debe ser entero y mayor o igual que
  cero.
- `loop 0` se elimina al optimizar; los loops positivos se conservan aunque
  tengan el cuerpo vacío.

Ejemplo de comentario:

```text
// Comentario de línea
tempo 120; // Comentario después de una instrucción
```

## Ejemplo Representativo del Lenguaje

A continuación se muestra un programa válido escrito en nuestro DSL que demuestra el uso de control de tempo, bucles anidados, cambios de octava dentro del rango de 88 teclas y estructuras redundantes que son procesadas por el optimizador:

```text
// Saria's Song (Lost Woods) - The Legend of Zelda: Ocarina of Time
tempo 140;

loop 2 {
    play F4, 0.5;
    play A4, 0.5;
    play B4, 1.0;
    
    play F4, 0.5;
    play A4, 0.5;
    play B4, 1.0;
    
    play F4, 0.5;
    play A4, 0.5;
    play B4, 0.5;
    play E5, 0.5;
    play D5, 1.0;
    
    play B4, 0.5;
    play C5, 0.5;
    play B4, 0.5;
    play G4, 0.5;
    play E4, 1.5;
    
    // Silencios consecutivos (se fusionan en un solo rest 0.5 al optimizar)
    rest 0.25;
    rest 0.25;
    
    play D4, 0.5;
    play E4, 0.5;
    play G4, 0.5;
    play E4, 1.5;
    
    rest 0.5;
}

// Bucle muerto (se elimina del AST al optimizar)
loop 0 {
    play C8, 1.0;
}
```

## 6. Optimizaciones

Las optimizaciones se implementan mediante `OptimizerVisitor` y modifican el
AST directamente.

### Fusión de Silencios

Dos o más nodos `Rest` consecutivos se convierten en un único `Rest`, sumando
sus duraciones:

```text
rest 0.25;
rest 0.25;
```

Después de optimizar:

```text
rest 0.5;
```

La optimización funciona tanto en el programa principal como dentro de loops
anidados. Si la eliminación de un loop deja dos silencios juntos, también se
fusionan.

### Eliminación de Bucles Vacíos

Un loop con cero repeticiones no puede producir audio y se elimina de la lista
de instrucciones de su nodo padre:

```text
loop 0 {
    play C8, 1.0;
}
```

Después de optimizar, el bloque desaparece completamente del AST.

Un loop con repeticiones positivas se conserva, incluso si su cuerpo está
vacío, porque la optimización implementada solo elimina loops cuyo número de
repeticiones es exactamente cero.

## Arquitectura del compilador

### AST

El AST se define en `src/ast.h` y contiene:

- `Program`: lista de instrucciones.
- `Tempo`: BPM entero.
- `Play`: nota y duración.
- `Rest`: duración.
- `Loop`: repeticiones y lista de instrucciones.

Todos los nodos heredan de `Node` y exponen:

```cpp
virtual void accept(Visitor* visitor) = 0;
```

### Visitor

`Visitor` define un método `visit` para cada tipo de nodo. Esto permite añadir
fases nuevas sin modificar la estructura del AST:

- `TypeCheckerVisitor`
- `OptimizerVisitor`
- `GenCodeVisitor`
- `PrintASTVisitor`

### Flujo completo

```text
Archivo .music
      |
      v
Lexer -> Parser -> AST
                   |
                   v
          TypeCheckerVisitor
                   |
                   v
          OptimizerVisitor
                   |
                   v
           GenCodeVisitor
                   |
                   v
             <cancion>.py
```

## Manejo de errores

Los errores léxicos y sintácticos se lanzan como `std::runtime_error` e incluyen
línea y columna:

```text
error de sintaxis (línea 2, columna 10): se esperaba ';'
```

Los errores semánticos incluyen, entre otros:

- Nota fuera del rango `A0`–`C8`.
- Duración menor o igual que cero.
- Repeticiones negativas.

El CLI devuelve código de salida `1` cuando ocurre un error.

## Limitaciones conocidas

- La reproducción de audio depende de `winsound`, por lo que está orientada a
  Windows.
- El generador produce un mapa de frecuencias de piano con valores aproximados.
- La validación musical se limita al rango y formato de las notas; no comprueba
  reglas avanzadas de armonía o composición.
- Las carpetas `build/` y `generated/` son artefactos locales y no deben
  versionarse.
