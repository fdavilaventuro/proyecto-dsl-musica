import shutil
import subprocess
import sys
import tkinter as tk
from pathlib import Path
from tkinter import filedialog, ttk


PROJECT_DIR = Path(__file__).resolve().parent.parent
BUILD_DIR = PROJECT_DIR / "build"
GENERATED_DIR = PROJECT_DIR / "generated"
LAST_OPENED_PATH = GENERATED_DIR / ".last_opened"
DEFAULT_SOURCE_PATH = PROJECT_DIR / "examples" / "sarias_song.music"
COMPILER_CANDIDATES = (
    "music_compiler.exe",
    "music_compiler",
    "musicc.exe",
    "musicc",
)

DEFAULT_SOURCE = """// Ejemplo del DSL musical
tempo 120;
play C4, 1;
rest 0.5;
loop 2 {
    play E4, 0.5;
    play G4, 0.5;
}
"""


class MusicIDE:
    def __init__(self, root):
        self.root = root
        self.current_file_path = (
            self._recover_last_opened_path() or DEFAULT_SOURCE_PATH
        )
        self.root.title("Music DSL IDE")
        self.root.geometry("1200x720")
        self.root.minsize(800, 500)
        self.root.bind("<Control-s>", self.save_file)
        self.root.bind("<Control-Shift-S>", self.save_file_as)

        self._configure_layout()
        self._create_toolbar()
        self._create_editor()
        self._create_output_tabs()

        self._load_initial_source()

    def _configure_layout(self):
        self.root.columnconfigure(0, weight=1)
        self.root.columnconfigure(1, weight=2)
        self.root.rowconfigure(1, weight=1)

    def _create_toolbar(self):
        toolbar = ttk.Frame(self.root, padding=(8, 8, 8, 4))
        toolbar.grid(row=0, column=0, columnspan=2, sticky="ew")
        toolbar.columnconfigure(7, weight=1)

        ttk.Button(toolbar, text="Abrir Archivo", command=self.open_file).grid(
            row=0, column=0, padx=(0, 6)
        )
        ttk.Button(toolbar, text="Guardar", command=self.save_file).grid(
            row=0, column=1, padx=(0, 6)
        )
        ttk.Button(toolbar, text="Guardar como", command=self.save_file_as).grid(
            row=0, column=2, padx=(0, 6)
        )
        ttk.Button(toolbar, text="Compilar", command=self.compile_program).grid(
            row=0, column=3, padx=(0, 6)
        )
        ttk.Button(toolbar, text="Ejecutar", command=self.execute_program).grid(
            row=0, column=4, padx=(0, 12)
        )
        self.optimize_var = tk.BooleanVar(value=True)
        ttk.Checkbutton(
            toolbar,
            text="Aplicar Optimizaciones",
            variable=self.optimize_var,
        ).grid(row=0, column=5, padx=(0, 12))
        self.input_label = ttk.Label(toolbar)
        self.input_label.grid(row=0, column=6, sticky="w")
        self.output_label = ttk.Label(toolbar)
        self.output_label.grid(row=0, column=8, sticky="e")

    def _update_path_labels(self):
        if self.current_file_path is None:
            self.root.title("Music DSL IDE - Sin archivo")
            self.input_label.configure(text="Archivo: Sin archivo")
            self.output_label.configure(text="")
            return

        output_path = self._output_path()
        self.root.title(f"Music DSL IDE - {self.current_file_path.name}")
        self.input_label.configure(
            text=f"Archivo: {self.current_file_path}"
        )
        self.output_label.configure(
            text=f"Salida: {output_path.relative_to(PROJECT_DIR)}"
        )

    def _create_editor(self):
        editor_frame = ttk.LabelFrame(self.root, text="Editor de Código", padding=6)
        editor_frame.grid(row=1, column=0, sticky="nsew", padx=(8, 4), pady=(4, 8))
        editor_frame.columnconfigure(0, weight=1)
        editor_frame.rowconfigure(0, weight=1)

        self.editor = tk.Text(
            editor_frame,
            wrap="none",
            undo=True,
            font=("Consolas", 11),
        )
        self.editor.grid(row=0, column=0, sticky="nsew")

        scrollbar = ttk.Scrollbar(
            editor_frame, orient="vertical", command=self.editor.yview
        )
        scrollbar.grid(row=0, column=1, sticky="ns")
        self.editor.configure(yscrollcommand=scrollbar.set)

    def _create_output_tabs(self):
        output_frame = ttk.LabelFrame(
            self.root, text="Resultados del compilador", padding=6
        )
        output_frame.grid(row=1, column=1, sticky="nsew", padx=(4, 8), pady=(4, 8))
        output_frame.columnconfigure(0, weight=1)
        output_frame.rowconfigure(0, weight=1)

        self.tabs = ttk.Notebook(output_frame)
        self.tabs.grid(row=0, column=0, sticky="nsew")

        self.ast_original_output = self._create_readonly_tab("AST Original")
        self.ast_optimized_output = self._create_readonly_tab("AST Optimizado")
        self.python_output = self._create_readonly_tab("Código Python Generado")
        self.error_output = self._create_readonly_tab("Consola de Errores")

    def _create_readonly_tab(self, title):
        frame = ttk.Frame(self.tabs, padding=4)
        frame.columnconfigure(0, weight=1)
        frame.rowconfigure(0, weight=1)
        self.tabs.add(frame, text=title)

        text = tk.Text(frame, wrap="none", state="disabled", font=("Consolas", 10))
        text.grid(row=0, column=0, sticky="nsew")

        scrollbar = ttk.Scrollbar(frame, orient="vertical", command=text.yview)
        scrollbar.grid(row=0, column=1, sticky="ns")
        text.configure(yscrollcommand=scrollbar.set)
        return text

    def _output_path(self):
        if self.current_file_path is None:
            raise RuntimeError("No hay un archivo musical asociado.")
        return GENERATED_DIR / f"{self.current_file_path.stem}.py"

    def _load_initial_source(self):
        self._load_source(self.current_file_path, persist=True)

    @staticmethod
    def _recover_last_opened_path():
        if LAST_OPENED_PATH.is_file():
            try:
                saved_path = LAST_OPENED_PATH.read_text(
                    encoding="utf-8",
                    errors="replace",
                ).strip()
                if saved_path:
                    candidate = Path(saved_path).expanduser()
                    if candidate.is_file():
                        return candidate.resolve()
            except OSError:
                return None
        return None

    def _load_source(self, path, persist):
        path = Path(path).resolve()
        try:
            source = path.read_text(encoding="utf-8")
        except (OSError, UnicodeError) as error:
            self._set_output(
                self.error_output,
                f"No se pudo abrir {path}:\n{error}\n",
            )
            self.tabs.select(3)
            return False

        self.editor.delete("1.0", tk.END)
        self.editor.insert("1.0", source)
        self.current_file_path = path
        self._update_path_labels()

        if persist:
            try:
                self._persist_current_file_path()
            except OSError as error:
                self._set_output(
                    self.error_output,
                    f"No se pudo guardar {LAST_OPENED_PATH}:\n{error}\n",
                )
                self.tabs.select(3)
                return False
        return True

    def open_file(self):
        selected_path = filedialog.askopenfilename(
            title="Abrir archivo musical",
            initialdir=str(
                self.current_file_path.parent
                if self.current_file_path is not None
                else DEFAULT_SOURCE_PATH.parent
            ),
            filetypes=[
                ("Archivos Music DSL", "*.music"),
            ],
        )
        if selected_path:
            self._load_source(selected_path, persist=True)

    def _persist_current_file_path(self):
        if self.current_file_path is None:
            return
        GENERATED_DIR.mkdir(parents=True, exist_ok=True)
        LAST_OPENED_PATH.write_text(
            str(self.current_file_path),
            encoding="utf-8",
        )

    def save_file(self, _event=None):
        if self.current_file_path is None:
            return self.save_file_as()

        try:
            self.current_file_path.write_text(
                self.editor.get("1.0", "end-1c"),
                encoding="utf-8",
            )
            self._persist_current_file_path()
        except (OSError, UnicodeError) as error:
            self._set_output(
                self.error_output,
                f"No se pudo guardar {self.current_file_path}:\n{error}\n",
            )
            self.tabs.select(3)
            return

        self._set_output(
            self.error_output,
            f"[Info] Archivo guardado: {self.current_file_path}\n",
        )
        self.tabs.select(3)

    def save_file_as(self, _event=None):
        initial_file = (
            self.current_file_path.name
            if self.current_file_path is not None
            else "untitled.music"
        )
        selected_path = filedialog.asksaveasfilename(
            title="Guardar archivo musical como",
            initialdir=str(DEFAULT_SOURCE_PATH.parent),
            initialfile=initial_file,
            defaultextension=".music",
            filetypes=[
                ("Archivos Music DSL", "*.music"),
            ],
        )
        if not selected_path:
            return

        selected_path = Path(selected_path).expanduser().resolve()
        if selected_path.suffix.lower() != ".music":
            selected_path = selected_path.with_suffix(".music")
        try:
            selected_path.write_text(
                self.editor.get("1.0", "end-1c"),
                encoding="utf-8",
            )
            self.current_file_path = selected_path
            self._update_path_labels()
            self._persist_current_file_path()
        except (OSError, UnicodeError) as error:
            self._set_output(
                self.error_output,
                f"No se pudo guardar {selected_path}:\n{error}\n",
            )
            self.tabs.select(3)
            return

        self._set_output(
            self.error_output,
            f"[Info] Archivo guardado: {self.current_file_path}\n",
        )
        self.tabs.select(3)

    @staticmethod
    def _set_output(widget, content):
        widget.configure(state="normal")
        widget.delete("1.0", tk.END)
        widget.insert("1.0", content)
        widget.configure(state="disabled")

    @staticmethod
    def _find_compiler():
        for candidate in COMPILER_CANDIDATES:
            for directory in (BUILD_DIR, PROJECT_DIR):
                local_path = directory / candidate
                if local_path.is_file():
                    return str(local_path)

            path_from_environment = shutil.which(candidate)
            if path_from_environment:
                return path_from_environment

        raise FileNotFoundError(
            "No se encontró el compilador. Compile el proyecto como "
            "'music_compiler' o 'musicc' antes de usar el IDE."
        )

    def _run_compiler(self, compiler, flags):
        return subprocess.run(
            [compiler, str(self.current_file_path), *flags],
            cwd=str(PROJECT_DIR),
            capture_output=True,
            text=True,
            encoding="utf-8",
            errors="replace",
            check=False,
        )

    @staticmethod
    def _format_compiler_errors(label, result):
        messages = []
        if result.stderr:
            messages.append(f"{label}:\n{result.stderr}")
        if result.returncode != 0:
            messages.append(
                f"{label} terminó con código {result.returncode}."
            )
        return "\n".join(messages)

    def compile_program(self):
        self._set_output(self.ast_original_output, "")
        self._set_output(self.ast_optimized_output, "")
        self._set_output(self.python_output, "")
        self._set_output(self.error_output, "")

        if self.current_file_path is None:
            self.save_file_as()
            if self.current_file_path is None:
                return

        output_path = self._output_path()

        try:
            GENERATED_DIR.mkdir(parents=True, exist_ok=True)
            self.current_file_path.write_text(
                self.editor.get("1.0", "end-1c"),
                encoding="utf-8",
            )
            self._persist_current_file_path()
            if output_path.is_file():
                output_path.unlink()

            compiler = self._find_compiler()
            original_result = self._run_compiler(compiler, ["--print-ast"])
            self._set_output(
                self.ast_original_output,
                original_result.stdout,
            )
            if original_result.returncode != 0:
                self._set_output(
                    self.error_output,
                    self._format_compiler_errors(
                        "Compilación del AST original",
                        original_result,
                    ),
                )
                self.tabs.select(3)
                return

            final_result = original_result
            errors = self._format_compiler_errors(
                "Compilación original",
                original_result,
            )
            if self.optimize_var.get():
                if output_path.is_file():
                    output_path.unlink()
                optimized_result = self._run_compiler(
                    compiler,
                    ["--optimize", "--print-opt-ast"],
                )
                self._set_output(
                    self.ast_optimized_output,
                    optimized_result.stdout,
                )
                final_result = optimized_result
                optimized_errors = self._format_compiler_errors(
                    "Compilación optimizada",
                    optimized_result,
                )
                if optimized_errors:
                    errors = "\n".join(
                        message for message in (errors, optimized_errors) if message
                    )
            else:
                self._set_output(
                    self.ast_optimized_output,
                    "Optimizaciones desactivadas; el AST final coincide "
                    "con el AST original.\n\n" + original_result.stdout,
                )

            generated_code = ""
            if final_result.returncode == 0 and output_path.is_file():
                generated_code = output_path.read_text(
                    encoding="utf-8",
                    errors="replace",
                )
            self._set_output(self.python_output, generated_code)

            if final_result.returncode == 0 and not generated_code:
                errors = "\n".join(
                    message
                    for message in (
                        errors,
                        f"La compilación terminó sin generar {output_path.name}.",
                    )
                    if message
                )

            self._set_output(self.error_output, errors)
            self.tabs.select(3 if errors else 2)
        except (OSError, UnicodeError) as error:
            self._set_output(self.error_output, f"Error de compilación: {error}\n")
            self.tabs.select(3)

    def execute_program(self):
        output_path = self._output_path()
        if not output_path.is_file():
            message = (
                f"No existe {output_path.name}. "
                "Compila el programa antes de ejecutarlo."
            )
            self._set_output(self.error_output, message + "\n")
            self.tabs.select(3)
            return

        try:
            result = subprocess.run(
                [sys.executable, str(output_path)],
                cwd=str(PROJECT_DIR),
                capture_output=True,
                text=True,
                encoding="utf-8",
                errors="replace",
                check=False,
            )
            console = result.stdout
            if result.stderr:
                console += result.stderr
            if result.returncode != 0:
                console += (
                    f"\nEl programa terminó con código {result.returncode}.\n"
                )
            elif not console:
                console = "Ejecución finalizada correctamente.\n"

            self._set_output(self.error_output, console)
            self.tabs.select(3)
        except OSError as error:
            self._set_output(self.error_output, f"Error de ejecución: {error}\n")
            self.tabs.select(3)


def main():
    root = tk.Tk()
    MusicIDE(root)
    root.mainloop()


if __name__ == "__main__":
    main()
