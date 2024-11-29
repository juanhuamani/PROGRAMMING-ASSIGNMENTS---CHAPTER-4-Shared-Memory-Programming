import pandas as pd
import matplotlib.pyplot as plt

# Cargar los datos del archivo CSV
df = pd.read_csv('results.csv')

# Crear un gráfico de histogramas para los tiempos secuenciales y concurrentes
plt.figure(figsize=(10, 6))

# Histograma del tiempo secuencial
plt.subplot(1, 2, 1)
plt.hist(df['sequential'], bins=10, color='blue', alpha=0.7, label='Secuencial')
plt.xlabel('Tiempo (segundos)')
plt.ylabel('Frecuencia')
plt.title('Histograma de Tiempos Secuenciales')
plt.legend()

# Histograma del tiempo concurrente
plt.subplot(1, 2, 2)
plt.hist(df['concurrent'], bins=10, color='green', alpha=0.7, label='Concurrente')
plt.xlabel('Tiempo (segundos)')
plt.ylabel('Frecuencia')
plt.title('Histograma de Tiempos Concurrentes')
plt.legend()

# Guardar los gráficos
plt.tight_layout()
plt.savefig('histogramas.png')

# Crear un gráfico de barras para la mejora de rendimiento (Speedup)
plt.figure(figsize=(6, 6))
plt.bar(df['size'], df['speedup'], color='orange', alpha=0.7)
plt.xlabel('Tamaño de la lista')
plt.ylabel('Speedup')
plt.title('Speedup en función del tamaño de la lista')

# Guardar el gráfico de speedup
plt.savefig('speedup.png')
