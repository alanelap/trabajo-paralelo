# Variables para el compilador y las banderas
CXX = g++
CXXFLAGS = -fopenmp -Wall
LIBS = -lcurl

# Nombre de los ejecutables finales
TARGET_SEQ = tarea_secuencial
TARGET_PAR = tarea_paralela

# Regla por defecto: compila ambos programas
all: $(TARGET_SEQ) $(TARGET_PAR)

# Cómo compilar la versión secuencial
$(TARGET_SEQ): tarea_secuencial.cpp
	$(CXX) tarea_secuencial.cpp -o $(TARGET_SEQ) $(LIBS)

# Cómo compilar la versión paralela
$(TARGET_PAR): tarea_paralela.cpp
	$(CXX) $(CXXFLAGS) tarea_paralela.cpp -o $(TARGET_PAR) $(LIBS)

# Regla para limpiar los ejecutables y archivos temporales
clean:
	rm -f $(TARGET_SEQ) $(TARGET_PAR) resultados.txt log.txt
