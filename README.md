# **Projeto 2** - Simulador de Paginação de Memória

O presente repositório contem o nosso projeto que simula a paginação em memória virtual, um componente essencial dos sistemas operacionais modernos, aplicando dois algoritmos diferente o LRU e o FIFO.

Os integrantes do grupo são:

* DIOGO OLIVEIRA UEMA           RA:10426124
* ARTHUR SILVA SANTANA          RA:10420550
* MURILO HENRIQUE SAKAMOTO      RA:10426242  
* DANIEL MONTEIRO MALACARNE     RA:10420454

# Quais passos para rodar o projeto?

1. Clonar o repositório
'''
git clone https://github.com/Diogouema/ProjSistemaOperacional.git
'''
'''
cd src
'''

3. Verificar a Estrutura de Arquivos
📁 src
 ┣ 📁 output
 ┣ 📄 algoritmos.c
 ┣ 📄 algoritmos.h
 ┣ 📄 estruturas.h
 ┗ 📄 simulador.c

4. Compilar o projeto

Linux/macOS:
'gcc simulador.c algoritmos.c -o simulador'
 
Windows (usando MinGW):
'gcc simulador.c algoritmos.c -o simulador'

4. Executar o Programa

Linux/macOS:
'./simulador'

Windows:
'simulador.exe'
