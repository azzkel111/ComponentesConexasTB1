#pragma once
// ============================================================
//  Grafo.h
//  Modelo de datos: grafo representado con su matriz de
//  adyacencia. Puede ser no dirigido (matriz simetrica) o
//  dirigido (arcos con flecha), y admite lazos (a_ii = 1).
// ============================================================

namespace ComponentesConexas {

	using namespace System;

	public ref class Grafo {
	public:
		static const int MIN_NODOS = 4;
		static const int MAX_NODOS = 12;

		Grafo(int nodos) { Iniciar(nodos, false); }
		Grafo(int nodos, bool esDirigido) { Iniciar(nodos, esDirigido); }

		property int N { int get() { return n; } }

		// Al pasar a no dirigido cada arco i->j se convierte en la arista {i, j}
		// (A = A OR A^T); al pasar a dirigido cada arista queda como dos arcos.
		property bool Dirigido {
			bool get() { return dirigido; }
			void set(bool v) {
				if (!v)
					for (int i = 0; i < n; i++)
						for (int j = 0; j < n; j++)
							if (ady[i, j] == 1) ady[j, i] = 1;
				dirigido = v;
			}
		}

		int Ady(int i, int j) { return ady[i, j]; }

		// Hay conexion entre i y j sin importar la direccion del arco.
		bool Conectados(int i, int j) { return ady[i, j] == 1 || ady[j, i] == 1; }

		bool Valido(int i) { return i >= 0 && i < n; }

		// Crea o quita la arista {i, j} (o el arco i -> j si es dirigido).
		// Con i == j crea o quita un lazo. Devuelve el nuevo valor (0/1).
		int AlternarArista(int i, int j) {
			if (!Valido(i) || !Valido(j)) return 0;
			int v = 1 - ady[i, j];
			ady[i, j] = v;
			if (!dirigido) ady[j, i] = v;
			return v;
		}

		void PonerArista(int i, int j) {
			if (!Valido(i) || !Valido(j)) return;
			ady[i, j] = 1;
			if (!dirigido) ady[j, i] = 1;
		}

		void Limpiar() {
			for (int i = 0; i < n; i++)
				for (int j = 0; j < n; j++) ady[i, j] = 0;
		}

		// Cada par {i, j} recibe una conexion con probabilidad p. En un grafo
		// dirigido la flecha va i->j o j->i (y a veces en ambos sentidos).
		// Cada nodo recibe un lazo con probabilidad pLazo.
		void Aleatorio(Random^ rnd, double p, double pLazo) {
			Limpiar();
			for (int i = 0; i < n; i++) {
				if (rnd->NextDouble() < pLazo) ady[i, i] = 1;
				for (int j = i + 1; j < n; j++) {
					if (rnd->NextDouble() >= p) continue;
					if (!dirigido) { PonerArista(i, j); continue; }
					double s = rnd->NextDouble();
					if (s < 0.45) ady[i, j] = 1;
					else if (s < 0.9) ady[j, i] = 1;
					else { ady[i, j] = 1; ady[j, i] = 1; }
				}
			}
		}

		void Aleatorio(Random^ rnd, double p) { Aleatorio(rnd, p, 0.0); }

		// Aristas (no dirigido) o arcos (dirigido), contando los lazos.
		int ContarAristas() {
			int m = 0;
			for (int i = 0; i < n; i++)
				for (int j = dirigido ? 0 : i; j < n; j++) m += ady[i, j];
			return m;
		}

		int ContarLazos() {
			int m = 0;
			for (int i = 0; i < n; i++) m += ady[i, i];
			return m;
		}

		// Copia de la matriz (los pasos del algoritmo trabajan sobre copias).
		array<int, 2>^ CopiaMatriz() {
			return (array<int, 2>^)ady->Clone();
		}

	private:
		void Iniciar(int nodos, bool esDirigido) {
			if (nodos < MIN_NODOS || nodos > MAX_NODOS)
				throw gcnew ArgumentOutOfRangeException("nodos", "El grafo debe tener entre 4 y 12 nodos.");
			n = nodos;
			dirigido = esDirigido;
			ady = gcnew array<int, 2>(n, n);
		}

		int n;
		bool dirigido;
		array<int, 2>^ ady;
	};
}
