#include <iostream>
#include <string>
#include <curl/curl.h>

size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

int main() {
    CURL* curl;
    CURLcode res;
    std::string readBuffer;
    long http_code = 0;

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();

    if(curl) {
        // ¡CAMBIADO! Usamos el UUID de la Fila 2 para probar otro registro
        std::string uuid_prueba = "a6466244-8c40-3c4a-8fae-16eae03c1272";
        std::string url_completa = "http://localhost:8080/cpyd/person/" + uuid_prueba;

        curl_easy_setopt(curl, CURLOPT_URL, url_completa.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

        std::cout << "Consultando datos del cliente en la API REST..." << std::endl;

        res = curl_easy_perform(curl);

        if(res != CURLE_OK) {
            std::cerr << "Error de red cURL: " << curl_easy_strerror(res) << std::endl;
        } else {
            // Obtenemos el código de respuesta HTTP del servidor (ej: 200, 404, 500)
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
            
            std::cout << "\nCódigo de Estado HTTP: " << http_code << std::endl;
            std::cout << "--- RESPUESTA DE LA API ---" << std::endl;
            std::cout << readBuffer << std::endl;
            std::cout << "---------------------------" << std::endl;
        }

        curl_easy_cleanup(curl);
    }

    curl_global_cleanup();
    return 0;
}

