#pragma once
// ============================================================
//  Algoritmo.h
//  Componentes conexas con el procedimiento de la Lectura 5.1:
//   1) M = A + I  (matriz de adyacencia con 1 en la diagonal)
//   2) Matriz de caminos M_R∞ a partir de M
//   3) Contar los 1 de cada fila y ordenar las filas de mayor a
//      menor (empate: primero la que tiene el 1 mas a la izquierda)
//   4) Ordenar las columnas igual que las filas: cada bloque
//      cuadrado diagonal de 1 es una componente conexa.
//  El paso 2 se puede calcular de dos formas (el usuario elige):
//   - Warshall: el nodo k se usa como intermedio, k = 1..n
//   - DFS: desde cada nodo se recorren las flechas con una pila
//  Los algoritmos no dibujan: generan una lista de "pasos"
//  (fotografias del estado) que la interfaz recorre.
// ============================================================

#include "Grafo.h"
#using <System.dll>   // Stack<T> esta en System.dll

namespace ComponentesConexas {

	using namespace System;
	using namespace System::Collections::Generic;
	using namespace System::Text;

	public enum class TipoPaso {
		Inicio, Diagonal, Warshall, FilaDfs, Conteo, OrdenFilas, OrdenColumnas, Bloque, Fin
	};

	// Fotografia del estado del algoritmo en un instante.
	public ref class Paso {
	public:
		TipoPaso tipo;
		String^ titulo;            // resumen corto (lista de pasos)
		String^ texto;             // explicacion detallada
		array<int, 2>^ matriz;     // matriz (indices de nodo, sin reordenar)
		array<bool, 2>^ nuevos;    // celdas que cambiaron en este paso
		array<int>^ comp;          // componente de cada nodo (-1 = sin asignar)
		array<int>^ orden;         // orden de filas para mostrar (nullptr = 1..n)
		bool ordenColumnas;        // las columnas tambien usan "orden"
		bool mostrarConteo;        // mostrar la cantidad de 1 de cada fila
		array<int>^ recorrido;     // DFS: nodos en el orden en que se visitaron
		int filaTrabajo;           // fila del nodo de origen (DFS), azul
		int intermedio;            // Warshall: nodo k (fila y columna k), ambar
		int nodoActual;            // nodo resaltado en el grafo
		int bloque;                // componente que se esta marcando (-1 = ninguna)
		bool diagonal;             // resaltar la diagonal principal
	};

	public ref class Resultado {
	public:
		String^ metodo;
		List<Paso^>^ pasos;
		List<List<int>^>^ componentes;  // en el orden de los bloques diagonales
		array<int>^ comp;
		array<int, 2>^ caminos;         // matriz de caminos M_R∞
		array<int>^ orden;              // orden final de filas y columnas
	};

	public ref class Algoritmos abstract sealed {
	public:
		// ---------------- Texto auxiliar (nodos numerados desde 1) ----------------
		static String^ Conjunto(List<int>^ nodos) {
			StringBuilder^ sb = gcnew StringBuilder("{");
			for (int k = 0; k < nodos->Count; k++) {
				if (k > 0) sb->Append(", ");
				sb->Append(nodos[k] + 1);
			}
			return sb->Append("}")->ToString();
		}

		static String^ Fila(array<int, 2>^ m, int fila) {
			int n = m->GetLength(0);
			StringBuilder^ sb = gcnew StringBuilder("[");
			for (int j = 0; j < n; j++) {
				if (j > 0) sb->Append(" ");
				sb->Append(m[fila, j]);
			}
			return sb->Append("]")->ToString();
		}

		static int ContarUnos(array<int, 2>^ m, int fila) {
			int c = 0;
			for (int j = 0; j < m->GetLength(1); j++) c += m[fila, j];
			return c;
		}

		// ============================================================
		//  METODO 1: matriz de caminos con Warshall
		// ============================================================
		static Resultado^ Matricial(Grafo^ g) {
			int n = g->N;
			Contexto^ cx = gcnew Contexto(n, g->CopiaMatriz());
			array<int, 2>^ M = cx->M;
			PasosIniciales(cx, g);

			for (int k = 0; k < n; k++) {
				List<String^>^ agregados = gcnew List<String^>();
				List<int>^ filas = gcnew List<int>();
				for (int i = 0; i < n; i++) {
					if (M[i, k] == 0 || i == k) continue;
					filas->Add(i);
					for (int j = 0; j < n; j++)
						if (M[k, j] == 1 && M[i, j] == 0) {
							M[i, j] = 1;
							cx->nuevos[i, j] = true;
							agregados->Add("(" + (i + 1).ToString() + "," + (j + 1).ToString() + ")");
						}
				}
				cx->intermedio = k;
				cx->nodoActual = k;
				String^ efecto = agregados->Count == 0
					? "No aparecen caminos nuevos."
					: "Caminos nuevos en " + String::Join(", ", agregados) + ".";
				cx->Guardar(TipoPaso::Warshall, "Paso 2 · k = " + (k + 1).ToString() + " como intermedio" +
					(agregados->Count > 0 ? "  →  " + agregados->Count.ToString() + " uno(s) nuevo(s)" : "  →  sin cambios"),
					"Si i llega a " + (k + 1).ToString() + " y " + (k + 1).ToString() + " llega a j, entonces i llega a j. "
					"A cada fila con 1 en la columna " + (k + 1).ToString() + (filas->Count > 0 ? " (filas " + Lista(filas) + ")" : " (ninguna)") +
					" se le suma (OR) la fila " + (k + 1).ToString() + " = " + Fila(M, k) + ". " + efecto);
			}
			cx->intermedio = -1;
			cx->nodoActual = -1;
			return PasosFinales(cx, "Warshall");
		}

		// ============================================================
		//  METODO 2: matriz de caminos con un DFS desde cada nodo
		// ============================================================
		static Resultado^ Dfs(Grafo^ g) {
			int n = g->N;
			array<int, 2>^ A = g->CopiaMatriz();
			Contexto^ cx = gcnew Contexto(n, g->CopiaMatriz());
			array<int, 2>^ M = cx->M;
			PasosIniciales(cx, g);

			for (int s = 0; s < n; s++) {
				array<bool>^ visto = gcnew array<bool>(n);
				List<int>^ orden = gcnew List<int>();
				Stack<int>^ pila = gcnew Stack<int>();
				pila->Push(s);
				visto[s] = true;
				while (pila->Count > 0) {
					int u = pila->Pop();
					orden->Add(u);
					for (int v = n - 1; v >= 0; v--)      // se apilan al reves para visitar en orden creciente
						if (A[u, v] == 1 && !visto[v]) { visto[v] = true; pila->Push(v); }
				}
				List<int>^ nuevos = gcnew List<int>();
				for (int j = 0; j < n; j++)
					if (visto[j] && M[s, j] == 0) { M[s, j] = 1; cx->nuevos[s, j] = true; nuevos->Add(j); }
				cx->filaTrabajo = s;
				cx->nodoActual = s;
				cx->recorrido = orden->ToArray();
				StringBuilder^ r = gcnew StringBuilder();
				for (int t = 0; t < orden->Count; t++) { if (t > 0) r->Append(" → "); r->Append(orden[t] + 1); }
				List<int>^ alcanzados = UnosDeFila(M, s);
				cx->Guardar(TipoPaso::FilaDfs, "Paso 2 · DFS desde " + (s + 1).ToString() + "  →  alcanza " + Conjunto(alcanzados),
					"Se recorre el grafo desde el nodo " + (s + 1).ToString() + " siguiendo las flechas con una pila (orden de visita: " + r->ToString() +
					"). La fila " + (s + 1).ToString() + " de la matriz de caminos es " + Fila(M, s) +
					(nuevos->Count > 0 ? "; caminos de más de un arco: " + Conjunto(nuevos) + "." : "; no hay caminos de más de un arco."));
				cx->recorrido = nullptr;
			}
			cx->filaTrabajo = -1;
			cx->nodoActual = -1;
			return PasosFinales(cx, "DFS");
		}

		// ============================================================
		//  REFERENCIA INDEPENDIENTE (para validar): nodos que se alcanzan
		//  mutuamente, con alcanzabilidad calculada por fuerza bruta.
		//  Devuelve las componentes ordenadas por su menor nodo.
		// ============================================================
		static List<List<int>^>^ Referencia(Grafo^ g) {
			int n = g->N;
			array<bool, 2>^ alc = gcnew array<bool, 2>(n, n);
			for (int s = 0; s < n; s++) {
				alc[s, s] = true;
				bool cambio = true;
				while (cambio) {
					cambio = false;
					for (int u = 0; u < n; u++)
						if (alc[s, u])
							for (int v = 0; v < n; v++)
								if (g->Ady(u, v) && !alc[s, v]) { alc[s, v] = true; cambio = true; }
				}
			}
			List<List<int>^>^ res = gcnew List<List<int>^>();
			array<bool>^ usado = gcnew array<bool>(n);
			for (int i = 0; i < n; i++) {
				if (usado[i]) continue;
				List<int>^ grupo = gcnew List<int>();
				for (int j = i; j < n; j++)
					if (!usado[j] && alc[i, j] && alc[j, i]) { grupo->Add(j); usado[j] = true; }
				res->Add(grupo);
			}
			return res;
		}

		// Componentes ordenadas por su menor nodo (para comparar resultados).
		static List<List<int>^>^ Canonico(List<List<int>^>^ comps) {
			List<List<int>^>^ c = gcnew List<List<int>^>();
			for each (List<int> ^ x in comps) { List<int>^ y = gcnew List<int>(x); y->Sort(); c->Add(y); }
			c->Sort(gcnew Comparison<List<int>^>(&Algoritmos::PorMenor));
			return c;
		}

		static bool Iguales(List<List<int>^>^ a, List<List<int>^>^ b) {
			if (a->Count != b->Count) return false;
			for (int k = 0; k < a->Count; k++) {
				if (a[k]->Count != b[k]->Count) return false;
				for (int t = 0; t < a[k]->Count; t++)
					if (a[k][t] != b[k][t]) return false;
			}
			return true;
		}

		// Orden del paso 3: mas 1 primero; en empate, la fila con el 1 mas a la
		// izquierda (se compara columna por columna); al final, el menor nodo.
		static array<int>^ OrdenFilas(array<int, 2>^ R) {
			int n = R->GetLength(0);
			List<int>^ orden = gcnew List<int>();
			for (int i = 0; i < n; i++) orden->Add(i);
			ComparadorFilas^ cmp = gcnew ComparadorFilas(R);
			orden->Sort(gcnew Comparison<int>(cmp, &ComparadorFilas::Comparar));
			return orden->ToArray();
		}

	private:
		ref class ComparadorFilas {
		public:
			array<int, 2>^ R;
			ComparadorFilas(array<int, 2>^ m) { R = m; }
			int Comparar(int a, int b) {
				int ca = ContarUnos(R, a), cb = ContarUnos(R, b);
				if (ca != cb) return cb - ca;
				for (int j = 0; j < R->GetLength(1); j++)
					if (R[a, j] != R[b, j]) return R[a, j] == 1 ? -1 : 1;
				return a - b;
			}
		};

		static int PorMenor(List<int>^ a, List<int>^ b) { return a[0] - b[0]; }

		// Estado mutable mientras se generan los pasos.
		ref class Contexto {
		public:
			int n;
			array<int, 2>^ M;
			array<bool, 2>^ nuevos;
			array<int>^ comp;
			array<int>^ orden;
			bool ordenColumnas, mostrarConteo, diagonal;
			array<int>^ recorrido;
			int filaTrabajo, intermedio, nodoActual, bloque;
			List<Paso^>^ pasos;
			List<List<int>^>^ componentes;

			Contexto(int nodos, array<int, 2>^ matriz) {
				n = nodos;
				M = matriz;
				nuevos = gcnew array<bool, 2>(n, n);
				comp = gcnew array<int>(n);
				for (int i = 0; i < n; i++) comp[i] = -1;
				filaTrabajo = intermedio = nodoActual = bloque = -1;
				ordenColumnas = mostrarConteo = diagonal = false;
				pasos = gcnew List<Paso^>();
				componentes = gcnew List<List<int>^>();
			}

			void Guardar(TipoPaso tipo, String^ titulo, String^ texto) {
				Paso^ p = gcnew Paso();
				p->tipo = tipo;
				p->titulo = titulo;
				p->texto = texto;
				p->matriz = (array<int, 2>^)M->Clone();
				p->nuevos = (array<bool, 2>^)nuevos->Clone();
				p->comp = (array<int>^)comp->Clone();
				p->orden = orden == nullptr ? nullptr : (array<int>^)orden->Clone();
				p->ordenColumnas = ordenColumnas;
				p->mostrarConteo = mostrarConteo;
				p->recorrido = recorrido == nullptr ? nullptr : (array<int>^)recorrido->Clone();
				p->filaTrabajo = filaTrabajo;
				p->intermedio = intermedio;
				p->nodoActual = nodoActual;
				p->bloque = bloque;
				p->diagonal = diagonal;
				pasos->Add(p);
				// Las celdas "nuevas" solo se resaltan en el paso donde aparecen.
				nuevos = gcnew array<bool, 2>(n, n);
			}
		};

		// Paso de inicio y paso 1 (M = A + I).
		static void PasosIniciales(Contexto^ cx, Grafo^ g) {
			int n = g->N;
			cx->Guardar(TipoPaso::Inicio, "Matriz de adyacencia A",
				"Grafo dirigido: un 1 en la celda (i, j) indica la flecha i → j (fila = origen, columna = destino); "
				"un 1 en la diagonal es un lazo. Ningún nodo tiene componente asignada todavía.");
			int lazos = 0;
			for (int i = 0; i < n; i++) {
				if (cx->M[i, i] == 1) lazos++;
				else { cx->M[i, i] = 1; cx->nuevos[i, i] = true; }
			}
			cx->diagonal = true;
			cx->Guardar(TipoPaso::Diagonal, "Paso 1 · Agregar 1 en la diagonal: M = A + I",
				"Se coloca el valor 1 en toda la diagonal: cada nodo llega a sí mismo (camino de longitud 0). Así, al calcular "
				"la matriz de caminos, cada nodo queda dentro de su propio bloque." +
				(lazos > 0 ? " Los " + lazos.ToString() + " nodo(s) con lazo ya tenían ese 1." : ""));
			cx->diagonal = false;
		}

		// Pasos 3 y 4: conteo, orden de filas, orden de columnas y bloques diagonales.
		static Resultado^ PasosFinales(Contexto^ cx, String^ metodo) {
			int n = cx->n;
			array<int, 2>^ R = cx->M;
			StringBuilder^ conteo = gcnew StringBuilder();
			for (int i = 0; i < n; i++) {
				if (i > 0) conteo->Append(", ");
				conteo->Append("fila " + (i + 1).ToString() + ": " + ContarUnos(R, i).ToString());
			}
			cx->mostrarConteo = true;
			cx->Guardar(TipoPaso::Conteo, "Paso 3 · Matriz de caminos lista: contar los 1 de cada fila",
				"La matriz de caminos M_R∞ indica con un 1 en (i, j) que existe un camino de i a j. Cantidad de 1 por fila: " + conteo->ToString() + ".");

			array<int>^ orden = OrdenFilas(R);
			cx->orden = orden;
			cx->Guardar(TipoPaso::OrdenFilas, "Paso 3 · Ordenar filas: " + ListaOrden(orden),
				"Las filas se ordenan de mayor a menor cantidad de 1. Si dos filas tienen la misma cantidad, va primero la que "
				"tiene el 1 más cercano a la primera columna. Nuevo orden de filas: " + ListaOrden(orden) + ".");

			cx->ordenColumnas = true;
			cx->Guardar(TipoPaso::OrdenColumnas, "Paso 4 · Ordenar columnas igual que las filas",
				"Las columnas se reordenan con el mismo orden " + ListaOrden(orden) + ". Ahora los 1 se agrupan en bloques "
				"cuadrados sobre la diagonal: cada bloque es una componente conexa.");

			// Bloques diagonales: filas consecutivas (en el nuevo orden) que son iguales.
			int c = 0;
			for (int p = 0; p < n; ) {
				int q = p + 1;
				while (q < n && FilasIguales(R, orden[p], orden[q])) q++;
				List<int>^ nodos = gcnew List<int>();
				for (int t = p; t < q; t++) { nodos->Add(orden[t]); cx->comp[orden[t]] = c; }
				nodos->Sort();
				cx->componentes->Add(nodos);
				cx->bloque = c;
				cx->Guardar(TipoPaso::Bloque, "Paso 4 · Bloque " + (c + 1).ToString() + ": V" + (c + 1).ToString() + " = " + Conjunto(nodos),
					"Bloque diagonal de tamaño " + (q - p).ToString() + "x" + (q - p).ToString() + " formado solo por 1" +
					(q - p > 1 ? ": estos nodos se alcanzan entre sí siguiendo las flechas." : ": el nodo no puede ir y volver con otro nodo.") +
					" Componente conexa V" + (c + 1).ToString() + " = " + Conjunto(nodos) + ".");
				c++;
				p = q;
			}
			cx->bloque = -1;
			cx->Guardar(TipoPaso::Fin, "Resultado: " + c.ToString() + " componente(s) conexa(s)", TextoFinal(cx->componentes));

			Resultado^ r = gcnew Resultado();
			r->metodo = metodo;
			r->pasos = cx->pasos;
			r->componentes = cx->componentes;
			r->comp = (array<int>^)cx->comp->Clone();
			r->caminos = (array<int, 2>^)R->Clone();
			r->orden = orden;
			return r;
		}

		static bool FilasIguales(array<int, 2>^ R, int a, int b) {
			for (int j = 0; j < R->GetLength(1); j++) if (R[a, j] != R[b, j]) return false;
			return true;
		}

		static List<int>^ UnosDeFila(array<int, 2>^ m, int fila) {
			List<int>^ l = gcnew List<int>();
			for (int j = 0; j < m->GetLength(1); j++) if (m[fila, j] == 1) l->Add(j);
			return l;
		}

		static String^ Lista(List<int>^ nodos) {
			StringBuilder^ sb = gcnew StringBuilder();
			for (int k = 0; k < nodos->Count; k++) { if (k > 0) sb->Append(", "); sb->Append(nodos[k] + 1); }
			return sb->ToString();
		}

		static String^ ListaOrden(array<int>^ orden) {
			StringBuilder^ sb = gcnew StringBuilder();
			for (int k = 0; k < orden->Length; k++) { if (k > 0) sb->Append(", "); sb->Append(orden[k] + 1); }
			return sb->ToString();
		}

		static String^ TextoFinal(List<List<int>^>^ comps) {
			StringBuilder^ sb = gcnew StringBuilder();
			sb->Append("El grafo tiene " + comps->Count.ToString() + " componente(s) conexa(s): ");
			for (int k = 0; k < comps->Count; k++) {
				if (k > 0) sb->Append("; ");
				sb->Append("V" + (k + 1).ToString() + " = " + Conjunto(comps[k]));
			}
			return sb->Append(".")->ToString();
		}
	};
}
