#include "app.hpp"

// Punto de entrada principal del editor predictivo médico bilingüe
int main() {
    App app;

    // Inicialización del sistema, persistencia y ventana gráfica
    app.init();

    // Bucle principal de la aplicación
    while (!app.should_close()) {
        app.update();
        app.render();
    }

    // Guardado de persistencia y liberación de recursos
    app.shutdown();

    return 0;
}
