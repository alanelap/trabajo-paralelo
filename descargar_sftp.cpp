#include <iostream>
#include <fstream>
#include <curl/curl.h>

// Función auxiliar que usa cURL para escribir el archivo en tu disco a medida que se descarga
size_t write_data(void *ptr, size_t size, size_t nmemb, FILE *stream) {
    size_t written = fwrite(ptr, size, nmemb, stream);
    return written;
}

int main() {
    CURL *curl;
    CURLcode res;

    // Inicializamos cURL
    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();

    if(curl) {
        // Configuramos la URL con el protocolo sftp, el host y la ruta del archivo de ejemplo (reporte_26128.csv)
        // Formato: sftp://usuario:contraseña@host/ruta_al_archivo
        curl_easy_setopt(curl, CURLOPT_URL, "sftp://utem:CPyD.2026@137.184.45.251/reporte_26128.csv");
        
	curl_easy_setopt(curl, CURLOPT_SSH_AUTH_TYPES, CURLSSH_AUTH_PASSWORD);
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
        // Abrimos un archivo local en modo escritura ("wb") para guardar los datos que bajen
        FILE *pagefile = fopen("reporte_26128.csv", "wb");
        if(pagefile) {
            // Le decimos a cURL que use nuestra función de escritura y guarde los datos en el archivo local
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, pagefile);

            std::cout << "Conectando al servidor SFTP y descargando archivo..." << std::endl;
            
            // Ejecutamos la operación de descarga
            res = curl_easy_perform(curl);
            
            // Cerramos el archivo local
            fclose(pagefile);

            // Verificamos si todo salió bien o si hubo algún error de red
            if(res != CURLE_OK) {
                std::cerr << "Error en la descarga: " << curl_easy_strerror(res) << std::endl;
            } else {
                std::cout << "¡Descarga completada con éxito! Revisa tu carpeta." << std::endl;
            }
        } else {
            std::cerr << "No se pudo crear el archivo local." << std::endl;
        }

        // Limpiamos los recursos de cURL
        curl_easy_cleanup(curl);
    }

    curl_global_cleanup();
    return 0;
}

