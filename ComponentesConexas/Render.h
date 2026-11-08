#pragma once
// ============================================================
//  Render.h
//  Todo el dibujo de la interfaz: grafo, matriz, resultado.
//  Las funciones solo leen el estado (Grafo + Paso) y pintan;
//  no modifican datos. La geometria se expone para que el
//  formulario pueda convertir clics en nodos / celdas.
// ============================================================

#include "Algoritmo.h"

namespace ComponentesConexas {

	using namespace System;
	using namespace System::Drawing;
	using namespace System::Drawing::Drawing2D;
	using namespace System::Windows::Forms;
	using namespace System::Collections::Generic;

	// ---------------- Colores y fuentes de la aplicacion ----------------
	public ref class Tema abstract sealed {
	public:
		static Color Fondo, Tarjeta, Borde, Texto, TextoSuave, Cabecera;
		static Color Acento, AcentoSuave, Ambar, AmbarSuave, Verde, VerdeSuave, Diagonal, Arista;
		static array<Color>^ Paleta;
		static Drawing::Font^ Titulo;
		static Drawing::Font^ Normal;
		static Drawing::Font^ Negrita;
		static Drawing::Font^ Pequena;

		static Tema() {
			Fondo = Color::FromArgb(241, 245, 249);
			Tarjeta = Color::White;
			Borde = Color::FromArgb(226, 232, 240);
			Texto = Color::FromArgb(15, 23, 42);
			TextoSuave = Color::FromArgb(100, 116, 139);
			Cabecera = Color::FromArgb(30, 41, 59);
			Acento = Color::FromArgb(37, 99, 235);
			AcentoSuave = Color::FromArgb(219, 234, 254);
			Ambar = Color::FromArgb(217, 119, 6);
			AmbarSuave = Color::FromArgb(254, 243, 199);
			Verde = Color::FromArgb(22, 163, 74);
			VerdeSuave = Color::FromArgb(187, 247, 208);
			Diagonal = Color::FromArgb(254, 240, 138);
			Arista = Color::FromArgb(148, 163, 184);
			Paleta = gcnew array<Color>{
				Color::FromArgb(37, 99, 235), Color::FromArgb(225, 29, 72), Color::FromArgb(22, 163, 74),
				Color::FromArgb(217, 119, 6), Color::FromArgb(124, 58, 237), Color::FromArgb(8, 145, 178),
				Color::FromArgb(219, 39, 119), Color::FromArgb(101, 163, 13), Color::FromArgb(234, 88, 12),
				Color::FromArgb(79, 70, 229), Color::FromArgb(13, 148, 136), Color::FromArgb(161, 98, 7) };
			Titulo = gcnew Drawing::Font("Segoe UI Semibold", 12.0f);
			Normal = gcnew Drawing::Font("Segoe UI", 9.75f);
			Negrita = gcnew Drawing::Font("Segoe UI", 9.75f, FontStyle::Bold);
			Pequena = gcnew Drawing::Font("Segoe UI", 8.5f);
		}

		static Color ColorComp(int c) { return Paleta[c % Paleta->Length]; }
		static Color Tenue(Color c, int alfa) { return Color::FromArgb(alfa, c); }
	};

	// Panel con doble buffer para dibujar sin parpadeo.
	public ref class Lienzo : public Panel {
	public:
		Lienzo() {
			SetStyle(ControlStyles::AllPaintingInWmPaint | ControlStyles::UserPaint |
				ControlStyles::OptimizedDoubleBuffer | ControlStyles::ResizeRedraw, true);
			BackColor = Tema::Tarjeta;
		}
	};

	// ---------------- Posicion de los nodos (coordenadas normalizadas 0..1) ----------------
	public ref class Disposicion abstract sealed {
	public:
		static array<PointF>^ Circular(int n) {
			array<PointF>^ p = gcnew array<PointF>(n);
			for (int i = 0; i < n; i++) {
				double a = 2 * Math::PI * i / n - Math::PI / 2;
				p[i] = PointF((float)(0.5 + 0.45 * Math::Cos(a)), (float)(0.5 + 0.45 * Math::Sin(a)));
			}
			return p;
		}

		// Disposicion por fuerzas (Fruchterman-Reingold): las aristas atraen,
		// todos los nodos se repelen. Separa visualmente las componentes y
		// reduce los cruces de aristas. Es determinista (misma entrada, mismo dibujo).
		static array<PointF>^ Fuerzas(Grafo^ g) {
			int n = g->N;
			array<double>^ x = gcnew array<double>(n);
			array<double>^ y = gcnew array<double>(n);
			Random^ r = gcnew Random(n);
			for (int i = 0; i < n; i++) {
				double a = 2 * Math::PI * i / n;
				x[i] = 0.5 + 0.35 * Math::Cos(a) + r->NextDouble() * 0.02;
				y[i] = 0.5 + 0.35 * Math::Sin(a) + r->NextDouble() * 0.02;
			}
			double k = 0.9 * Math::Sqrt(1.0 / n), t = 0.08;
			array<double>^ dx = gcnew array<double>(n);
			array<double>^ dy = gcnew array<double>(n);
			for (int it = 0; it < 500; it++) {
				for (int i = 0; i < n; i++) { dx[i] = (0.5 - x[i]) * 10.0 * k; dy[i] = (0.5 - y[i]) * 10.0 * k; }   // gravedad hacia el centro
				for (int i = 0; i < n; i++)
					for (int j = i + 1; j < n; j++) {
						double ex = x[i] - x[j], ey = y[i] - y[j];
						double d = Math::Max(Math::Sqrt(ex * ex + ey * ey), 0.01);
						double f = k * k / d - (g->Conectados(i, j) ? d * d / k : 0.0);
						dx[i] += ex / d * f; dy[i] += ey / d * f;
						dx[j] -= ex / d * f; dy[j] -= ey / d * f;
					}
				for (int i = 0; i < n; i++) {
					double len = Math::Sqrt(dx[i] * dx[i] + dy[i] * dy[i]);
					if (len > 1e-9) { x[i] += dx[i] / len * Math::Min(len, t); y[i] += dy[i] / len * Math::Min(len, t); }
				}
				t = Math::Max(t * 0.99, 0.002);
			}
			// Escala uniforme para ocupar el area disponible.
			double minX = x[0], maxX = x[0], minY = y[0], maxY = y[0];
			for (int i = 1; i < n; i++) {
				minX = Math::Min(minX, x[i]); maxX = Math::Max(maxX, x[i]);
				minY = Math::Min(minY, y[i]); maxY = Math::Max(maxY, y[i]);
			}
			double esc = 1.0 / Math::Max(Math::Max(maxX - minX, maxY - minY), 1e-6);
			array<PointF>^ p = gcnew array<PointF>(n);
			for (int i = 0; i < n; i++) {
				double px = 0.5 + (x[i] - (minX + maxX) / 2) * esc;
				double py = 0.5 + (y[i] - (minY + maxY) / 2) * esc;
				p[i] = PointF((float)px, (float)py);
			}
			return p;
		}
	};

	// ============================================================
	//  DIBUJO
	// ============================================================
	public ref class Render abstract sealed {
	public:
		// ---------------- Geometria del grafo ----------------
		static Rectangle AreaGrafo(Rectangle c) { return Rectangle(c.X + 10, c.Y + 44, c.Width - 20, c.Height - 44 - 34); }

		static float Radio(Rectangle area, int n) {
			float r = Math::Min(area.Width, area.Height) / (n <= 8 ? 13.0f : 16.0f);
			return Math::Max(16.0f, Math::Min(26.0f, r));
		}

		static PointF APixel(PointF p, Rectangle area, float m) {
			return PointF(area.X + m + p.X * (area.Width - 2 * m), area.Y + m + p.Y * (area.Height - 2 * m));
		}

		static PointF ANormal(Point px, Rectangle area, float m) {
			float w = Math::Max(1.0f, area.Width - 2 * m), h = Math::Max(1.0f, area.Height - 2 * m);
			float x = (px.X - area.X - m) / w, y = (px.Y - area.Y - m) / h;
			return PointF(Math::Max(0.0f, Math::Min(1.0f, x)), Math::Max(0.0f, Math::Min(1.0f, y)));
		}

		// Nodo bajo el punto (-1 si ninguno). Se revisa del ultimo al primero (el que se dibuja encima).
		static int NodoEn(Point pt, Rectangle cliente, array<PointF>^ pos) {
			if (pos == nullptr) return -1;
			Rectangle area = AreaGrafo(cliente);
			float R = Radio(area, pos->Length);
			for (int i = pos->Length - 1; i >= 0; i--) {
				PointF c = APixel(pos[i], area, R + 8);
				float dx = pt.X - c.X, dy = pt.Y - c.Y;
				if (dx * dx + dy * dy <= (R + 3) * (R + 3)) return i;
			}
			return -1;
		}

		// ---------------- Geometria de la matriz ----------------
		// A la derecha de la matriz van las columnas "nº 1" y "comp" y, mas a
		// la derecha, una barra lateral con la leyenda y datos del paso.
		static const int ANCHO_LATERAL = 190;

		static void GeoMatriz(Rectangle c, int n, int% x0, int% y0, int% celda) {
			int anchoDisp = c.Width - 40 - ANCHO_LATERAL - 20;
			int altoDisp = c.Height - 48 - 10;
			celda = (int)Math::Min(anchoDisp / (n + 3.6), altoDisp / (double)(n + 1));
			celda = Math::Max(16, Math::Min(44, celda));
			x0 = c.X + 40 + Math::Max(0, (anchoDisp - (int)(celda * (n + 3.6))) / 2);
			y0 = c.Y + 48;
		}

		static bool CeldaEn(Point pt, Rectangle c, int n, int% i, int% j) {
			int x0, y0, s;
			GeoMatriz(c, n, x0, y0, s);
			int cx = pt.X - (x0 + s), cy = pt.Y - (y0 + s);
			if (cx < 0 || cy < 0) return false;
			j = cx / s; i = cy / s;
			return i < n && j < n;
		}

		// ============================================================
		//  GRAFO
		// ============================================================
		static void DibujarGrafo(Graphics^ g, Rectangle cliente, Grafo^ grafo, array<PointF>^ pos, Paso^ paso,
			int seleccionado, int hover, String^ estado) {
			g->SmoothingMode = SmoothingMode::AntiAlias;
			g->TextRenderingHint = Drawing::Text::TextRenderingHint::ClearTypeGridFit;
			g->Clear(Tema::Tarjeta);
			Encabezado(g, cliente, "Grafo", estado);

			if (grafo == nullptr || pos == nullptr) {
				Centrado(g, "Elige la cantidad de nodos y pulsa «Crear grafo»", Tema::Normal, Tema::TextoSuave, cliente);
				return;
			}
			int n = grafo->N;
			Rectangle area = AreaGrafo(cliente);
			float R = Radio(area, n);
			array<PointF>^ p = gcnew array<PointF>(n);
			for (int i = 0; i < n; i++) p[i] = APixel(pos[i], area, R + 8);

			// --- DFS: flechas dentro de lo alcanzado desde el origen (verde) ---
			array<bool, 2>^ resaltar = gcnew array<bool, 2>(n, n);
			if (paso != nullptr && paso->tipo == TipoPaso::FilaDfs && paso->filaTrabajo >= 0)
				for (int a = 0; a < n; a++)
					for (int b = 0; b < n; b++)
						if (a != b && paso->matriz[paso->filaTrabajo, a] == 1 && paso->matriz[paso->filaTrabajo, b] == 1)
							resaltar[a, b] = true;

			// --- Aristas, arcos con flecha y lazos ---
			PointF centro((float)(area.X + area.Width / 2), (float)(area.Y + area.Height / 2));
			bool dir = grafo->Dirigido;
			for (int i = 0; i < n; i++)
				for (int j = dir ? 0 : i; j < n; j++) {
					if (grafo->Ady(i, j) == 0) continue;
					Color col = Tema::Arista;
					float ancho = 2.5f;
					if (paso != nullptr && paso->comp[i] >= 0 && paso->comp[i] == paso->comp[j]) {
						col = Tema::ColorComp(paso->comp[i]);
						ancho = 3.5f;
					}
					if (resaltar[i, j]) { col = Tema::Verde; ancho = 5.5f; }
					if (i == j) DibujarLazo(g, p[i], centro, R, col, ancho, dir);
					else {
						// Si hay flechas en ambos sentidos se dibujan dos arcos curvos separados.
						float curva = dir && grafo->Ady(j, i) == 1 ? R * 0.75f : 0.0f;
						DibujarArco(g, p[i], p[j], R, curva, col, ancho, dir);
					}
				}

			// --- Nodos ---
			for (int i = 0; i < n; i++) {
				int c = paso != nullptr ? paso->comp[i] : -1;
				// Anillos de estado
				if (paso != nullptr) {
					if (i == paso->filaTrabajo) Anillo(g, p[i], R + 6, Tema::Acento, 4.0f);
					if (i == paso->intermedio) Anillo(g, p[i], R + 6, Tema::Ambar, 4.0f);
					if (EsNuevo(paso, i)) Anillo(g, p[i], R + 6, Tema::Verde, 4.0f);
					if (paso->bloque >= 0 && paso->comp[i] == paso->bloque) Anillo(g, p[i], R + 6, ControlPaint::Dark(Tema::ColorComp(paso->bloque), 0.2f), 4.0f);
				}
				if (i == hover && i != seleccionado) Anillo(g, p[i], R + 5, Tema::AcentoSuave, 5.0f);

				RectangleF rc(p[i].X - R, p[i].Y - R, 2 * R, 2 * R);
				SolidBrush sombra(Color::FromArgb(35, 0, 0, 0));
				g->FillEllipse(%sombra, rc.X + 1.5f, rc.Y + 2.5f, rc.Width, rc.Height);
				SolidBrush relleno(c >= 0 ? Tema::ColorComp(c) : Color::White);
				g->FillEllipse(%relleno, rc);
				Pen borde(c >= 0 ? ControlPaint::Dark(Tema::ColorComp(c), 0.1f) : Tema::Arista, 2.0f);
				g->DrawEllipse(%borde, rc);
				if (i == seleccionado) {
					Pen sel(Tema::Acento, 3.0f);
					sel.DashStyle = DashStyle::Dash;
					g->DrawEllipse(%sel, rc.X - 5, rc.Y - 5, rc.Width + 10, rc.Height + 10);
				}
				Drawing::Font fuente("Segoe UI", R * 0.62f, FontStyle::Bold, GraphicsUnit::Pixel);
				Centrado(g, (i + 1).ToString(), %fuente, c >= 0 ? Color::White : Tema::Texto,
					Rectangle((int)rc.X, (int)rc.Y, (int)rc.Width + 1, (int)rc.Height + 1));
			}

			// --- Leyenda ---
			int ly = cliente.Bottom - 26, lx = cliente.X + 14;
			if (paso != nullptr) {
				lx = ItemLeyenda(g, lx, ly, Tema::Ambar, "Intermedio k (Warshall)", true);
				lx = ItemLeyenda(g, lx, ly, Tema::Acento, "Origen del DFS", true);
				lx = ItemLeyenda(g, lx, ly, Tema::Verde, "Alcanzado", true);
				ItemLeyenda(g, lx, ly, Tema::Arista, "Color = componente", false);
			}
			else {
				TextoEn(g, grafo->Dirigido
					? "Clic en un nodo y luego en otro: crea / quita la flecha.  Dos clics en el mismo nodo: lazo.  Arrastra para mover."
					: "Clic en un nodo y luego en otro: crea / quita la arista.  Dos clics en el mismo nodo: lazo.  Arrastra para mover.",
					Tema::Pequena, Tema::TextoSuave, lx, ly + 2);
			}
		}

		// ============================================================
		//  MATRIZ (con filas y columnas reordenadas en los pasos 3 y 4)
		// ============================================================
		static void DibujarMatriz(Graphics^ g, Rectangle cliente, Grafo^ grafo, Paso^ paso, bool warshall,
			bool final, int hi, int hj, bool editable) {
			g->SmoothingMode = SmoothingMode::AntiAlias;
			g->TextRenderingHint = Drawing::Text::TextRenderingHint::ClearTypeGridFit;
			g->Clear(Tema::Tarjeta);

			String^ titulo = "Matriz de adyacencia A";
			String^ sub = editable ? "Clic en (i, j): flecha i → j" : "";
			if (paso != nullptr) {
				switch (paso->tipo) {
				case TipoPaso::Inicio: sub = "fila = origen, columna = destino"; break;
				case TipoPaso::Diagonal: titulo = "Paso 1 · M = A + I"; sub = "1 en toda la diagonal"; break;
				case TipoPaso::Warshall: titulo = "Paso 2 · Matriz de caminos"; sub = "Warshall, k = " + (paso->intermedio + 1).ToString(); break;
				case TipoPaso::FilaDfs: titulo = "Paso 2 · Matriz de caminos"; sub = "DFS desde " + (paso->filaTrabajo + 1).ToString(); break;
				case TipoPaso::Conteo: titulo = "Paso 3 · Matriz de caminos M_R∞"; sub = "cantidad de 1 por fila"; break;
				case TipoPaso::OrdenFilas: titulo = "Paso 3 · Filas ordenadas"; sub = "de más a menos 1"; break;
				default: titulo = "Paso 4 · Bloques diagonales"; sub = "cada bloque de 1 = una componente"; break;
				}
			}
			Encabezado(g, cliente, titulo, sub);
			if (grafo == nullptr) return;

			int n = grafo->N, x0, y0, s;
			GeoMatriz(cliente, n, x0, y0, s);
			Drawing::Font celdaF("Segoe UI", Math::Max(9.0f, s * 0.42f), FontStyle::Bold, GraphicsUnit::Pixel);
			Drawing::Font celdaN("Segoe UI", Math::Max(9.0f, s * 0.42f), FontStyle::Regular, GraphicsUnit::Pixel);
			Drawing::Font cab("Segoe UI", Math::Max(9.0f, s * 0.38f), FontStyle::Bold, GraphicsUnit::Pixel);
			Drawing::Font chip("Segoe UI", Math::Max(8.0f, s * 0.36f), FontStyle::Bold, GraphicsUnit::Pixel);
			Pen rejilla(Tema::Borde, 1.0f);

			// Orden en que se muestran filas y columnas (pasos 3 y 4).
			array<int>^ ordF = gcnew array<int>(n);
			array<int>^ ordC = gcnew array<int>(n);
			for (int t = 0; t < n; t++) ordF[t] = ordC[t] = t;
			if (paso != nullptr && paso->orden != nullptr) {
				ordF = paso->orden;
				if (paso->ordenColumnas) ordC = paso->orden;
			}
			int fT = paso != nullptr ? paso->filaTrabajo : -1;
			int kk = paso != nullptr ? paso->intermedio : -1;

			// Cabeceras (muestran el numero de nodo de cada fila / columna)
			for (int t = 0; t < n; t++) {
				int nf = ordF[t], nc = ordC[t];
				Color fondoF = nf == fT ? Tema::Acento : (nf == kk ? Tema::Ambar : Tema::Fondo);
				Color fondoC = nc == kk ? Tema::Ambar : Tema::Fondo;
				Celda(g, Rectangle(x0, y0 + s * (t + 1), s, s), fondoF, (nf + 1).ToString(), %cab,
					nf == fT || nf == kk ? Color::White : Tema::TextoSuave);
				Celda(g, Rectangle(x0 + s * (t + 1), y0, s, s), fondoC, (nc + 1).ToString(), %cab,
					nc == kk ? Color::White : Tema::TextoSuave);
			}
			if (kk >= 0 || fT >= 0) {
				int fila = kk >= 0 ? kk : fT;
				int p = Array::IndexOf(ordF, fila);
				Rectangle et(x0 - 36, y0 + s * (p + 1) + s / 2 - 11, 32, 22);
				Redondeado(g, et, 6, kk >= 0 ? Tema::Ambar : Tema::Acento);
				Centrado(g, kk >= 0 ? "k" : "DFS", Tema::Pequena, Color::White, et);
			}

			array<int, 2>^ M = paso != nullptr ? paso->matriz : grafo->CopiaMatriz();
			for (int p = 0; p < n; p++)
				for (int q = 0; q < n; q++) {
					int i = ordF[p], j = ordC[q];
					Rectangle rc(x0 + s * (q + 1), y0 + s * (p + 1), s, s);
					Color fondo = Tema::Tarjeta;
					bool nuevo = false;
					if (paso != nullptr) {
						int ci = paso->comp[i];
						if (ci >= 0 && ci == paso->comp[j]) fondo = Tema::Tenue(Tema::ColorComp(ci), 70);
						if (paso->diagonal && i == j) fondo = Tema::Diagonal;
						if (i == fT) fondo = Tema::AcentoSuave;
						if (i == kk || j == kk) fondo = Tema::AmbarSuave;
						if (paso->nuevos[i, j]) { fondo = Tema::VerdeSuave; nuevo = true; }
					}
					else if (editable && i == hi && j == hj) fondo = Color::FromArgb(224, 231, 255);

					SolidBrush b(fondo);
					g->FillRectangle(%b, rc);
					g->DrawRectangle(%rejilla, rc);
					int v = M[i, j];
					Color ct = v == 1 ? (nuevo ? Color::FromArgb(21, 128, 61) : Tema::Texto) : Color::FromArgb(203, 213, 225);
					Centrado(g, v.ToString(), v == 1 ? %celdaF : %celdaN, ct, rc);
				}
			// Marcos: fila del origen (DFS), fila y columna k (Warshall)
			if (fT >= 0) {
				Pen marco(Tema::Acento, 2.5f);
				g->DrawRectangle(%marco, x0 + s, y0 + s * (Array::IndexOf(ordF, fT) + 1), s * n, s);
			}
			if (kk >= 0) {
				Pen marco(Tema::Ambar, 2.5f);
				g->DrawRectangle(%marco, x0 + s, y0 + s * (Array::IndexOf(ordF, kk) + 1), s * n, s);
				g->DrawRectangle(%marco, x0 + s * (Array::IndexOf(ordC, kk) + 1), y0 + s, s, s * n);
			}
			// Bloques diagonales (paso 4): un marco por componente ya identificada
			if (paso != nullptr && paso->orden != nullptr && paso->ordenColumnas) {
				for (int c = 0; ; c++) {
					int a = -1, z = -1;
					for (int p = 0; p < n; p++) if (paso->comp[ordF[p]] == c) { if (a < 0) a = p; z = p; }
					if (a < 0) break;
					Pen marco(Tema::ColorComp(c), c == paso->bloque ? 4.0f : 2.5f);
					g->DrawRectangle(%marco, x0 + s * (a + 1), y0 + s * (a + 1), s * (z - a + 1), s * (z - a + 1));
				}
			}

			// --- Columnas laterales: cantidad de 1 por fila y componente ---
			int xu = x0 + s * (n + 1) + (int)(s * 0.3);
			int xc = xu + s + (int)(s * 0.2);
			if (paso != nullptr && paso->mostrarConteo) {
				Centrado(g, "nº 1", %chip, Tema::TextoSuave, Rectangle(xu - 6, y0, s + 12, s));
				for (int p = 0; p < n; p++) {
					Rectangle rc(xu + 1, y0 + s * (p + 1) + 2, s - 2, s - 4);
					Redondeado(g, rc, 5, Tema::Cabecera);
					Centrado(g, Algoritmos::ContarUnos(M, ordF[p]).ToString(), %chip, Color::White, rc);
				}
			}
			Centrado(g, "comp", %chip, Tema::TextoSuave, Rectangle(xc - 6, y0, s + 12, s));
			for (int p = 0; p < n; p++) {
				Rectangle rc(xc + 1, y0 + s * (p + 1) + 2, s - 2, s - 4);
				int c = paso != nullptr ? paso->comp[ordF[p]] : -1;
				if (c >= 0) {
					Redondeado(g, rc, 5, Tema::ColorComp(c));
					Centrado(g, "V" + (c + 1).ToString(), %chip, Color::White, rc);
				}
				else {
					Redondeado(g, rc, 5, Tema::Fondo);
					Centrado(g, "–", %chip, Tema::TextoSuave, rc);
				}
			}

			// --- Barra lateral: leyenda e informacion del paso ---
			if (paso == nullptr) return;
			int lx = cliente.Right - ANCHO_LATERAL - 6, ly = y0 + 2;
			TextoEn(g, "Leyenda", Tema::Negrita, Tema::Texto, lx, ly);
			ly += 26;
			ItemLeyenda(g, lx, ly, Tema::Diagonal, "Diagonal (paso 1)", false); ly += 22;
			if (warshall) { ItemLeyenda(g, lx, ly, Tema::AmbarSuave, "Fila y columna k", false); ly += 22; }
			else { ItemLeyenda(g, lx, ly, Tema::AcentoSuave, "Fila del origen (DFS)", false); ly += 22; }
			ItemLeyenda(g, lx, ly, Tema::VerdeSuave, "Camino nuevo", false); ly += 22;
			ItemLeyenda(g, lx, ly, Tema::Tenue(Tema::Paleta[0], 70), "Bloque = componente", false);
			ly += 36;

			if (paso->tipo == TipoPaso::Warshall) {
				TextoEn(g, "Intermedio k = " + (kk + 1).ToString(), Tema::Negrita, Tema::Texto, lx, ly);
				TextoEn(g, "fila k = " + Algoritmos::Fila(M, kk), Tema::Normal, Tema::Ambar, lx, ly + 22);
			}
			else if (paso->recorrido != nullptr) {
				TextoEn(g, "Recorrido DFS (pila)", Tema::Negrita, Tema::Texto, lx, ly);
				ly += 26;
				int xp = lx;
				for (int t = 0; t < paso->recorrido->Length; t++) {
					if (xp + 30 > cliente.Right - 6) { xp = lx; ly += 32; }
					Rectangle rc(xp, ly, 28, 26);
					Redondeado(g, rc, 5, t == 0 ? Tema::Acento : Tema::Verde);
					Centrado(g, (paso->recorrido[t] + 1).ToString(), Tema::Negrita, Color::White, rc);
					xp += 32;
				}
			}
			else if (paso->orden != nullptr) {
				TextoEn(g, "Orden de filas" + (paso->ordenColumnas ? " y columnas" : ""), Tema::Negrita, Tema::Texto, lx, ly);
				TextoEn(g, OrdenTexto(paso->orden), Tema::Normal, Tema::Acento, lx, ly + 22);
			}
		}

		// ============================================================
		//  RESULTADO
		// ============================================================
		static void DibujarResultado(Graphics^ g, Rectangle cliente, Resultado^ res, int idx, bool coincide, String^ otro) {
			g->SmoothingMode = SmoothingMode::AntiAlias;
			g->TextRenderingHint = Drawing::Text::TextRenderingHint::ClearTypeGridFit;
			g->Clear(Tema::Tarjeta);
			if (res == nullptr) {
				TextoEn(g, "Resultado", Tema::Titulo, Tema::Texto, cliente.X + 14, cliente.Y + 8);
				TextoEn(g, "Inicia el algoritmo para ver cómo se forman las componentes conexas.",
					Tema::Normal, Tema::TextoSuave, cliente.X + 14, cliente.Y + 40);
				return;
			}
			bool fin = idx >= res->pasos->Count - 1;
			int k = 0;
			for (int t = 0; t <= idx && t < res->pasos->Count; t++)
				if (res->pasos[t]->tipo == TipoPaso::Bloque) k++;

			String^ titulo = fin
				? res->componentes->Count.ToString() + (res->componentes->Count == 1 ? " componente conexa" : " componentes conexas")
				: "Componentes encontradas: " + k.ToString();
			TextoEn(g, titulo, Tema::Titulo, fin ? Tema::Acento : Tema::Texto, cliente.X + 14, cliente.Y + 8);
			if (fin) {
				String^ v = coincide ? "✔ Verificado con " + otro + ": mismo resultado" : "✖ No coincide con " + otro;
				SizeF tam = g->MeasureString(v, Tema::Negrita);
				TextoEn(g, v, Tema::Negrita, coincide ? Tema::Verde : Color::Crimson,
					(int)(cliente.Right - tam.Width - 14), cliente.Y + 12);
			}

			int x = cliente.X + 14, y = cliente.Y + 42;
			for (int c = 0; c < k; c++) {
				String^ t = "V" + (c + 1).ToString() + " = " + Algoritmos::Conjunto(res->componentes[c]);
				int w = (int)g->MeasureString(t, Tema::Negrita).Width + 18;
				if (x + w > cliente.Right - 10 && x > cliente.X + 14) { x = cliente.X + 14; y += 30; }
				Rectangle rc(x, y, w, 25);
				Redondeado(g, rc, 12, Tema::ColorComp(c));
				Centrado(g, t, Tema::Negrita, Color::White, rc);
				x += w + 8;
			}
			if (k == 0) TextoEn(g, "Las componentes aparecen en el paso 4, al marcar los bloques diagonales.", Tema::Normal, Tema::TextoSuave, x, y + 3);
		}

	private:
		// Flecha proporcional al grosor del lapiz (la punta queda en el borde del nodo).
		static void PonerFlecha(Pen^ lapiz, float ancho) {
			float t = 11.0f / ancho;
			AdjustableArrowCap^ cap = gcnew AdjustableArrowCap(t * 0.85f, t, true);
			lapiz->CustomEndCap = cap;
		}

		// Conexion a -> b: recta (curva = 0) o curva cuadratica desplazada "curva" pixeles
		// a la izquierda del sentido a -> b. Se recorta en el borde de ambos nodos.
		static void DibujarArco(Graphics^ g, PointF a, PointF b, float R, float curva, Color col, float ancho, bool flecha) {
			float dx = b.X - a.X, dy = b.Y - a.Y;
			float L = (float)Math::Sqrt(dx * dx + dy * dy);
			if (L < 2 * R + 4) return;   // nodos superpuestos: no hay espacio para la linea
			PointF ctrl((a.X + b.X) / 2 - dy / L * curva * 2, (a.Y + b.Y) / 2 + dx / L * curva * 2);
			List<PointF>^ pts = gcnew List<PointF>();
			for (int k = 0; k <= 48; k++) {
				float t = k / 48.0f, u = 1 - t;
				PointF q(u * u * a.X + 2 * u * t * ctrl.X + t * t * b.X, u * u * a.Y + 2 * u * t * ctrl.Y + t * t * b.Y);
				if (Distancia(q, a) >= R + 1 && Distancia(q, b) >= R + (flecha ? 2 : 1)) pts->Add(q);
			}
			if (pts->Count < 2) return;
			Pen lapiz(col, ancho);
			lapiz.StartCap = LineCap::Round;
			if (flecha) PonerFlecha(%lapiz, ancho);
			g->DrawLines(%lapiz, pts->ToArray());
		}

		// Lazo: circulo pequeño pegado al nodo, en la direccion opuesta al centro del grafo.
		static void DibujarLazo(Graphics^ g, PointF c, PointF centro, float R, Color col, float ancho, bool flecha) {
			float dx = c.X - centro.X, dy = c.Y - centro.Y;
			float L = (float)Math::Sqrt(dx * dx + dy * dy);
			if (L < 1) { dx = 0; dy = -1; L = 1; }
			dx /= L; dy /= L;
			float r = R * 0.62f;
			PointF o(c.X + dx * R * 1.15f, c.Y + dy * R * 1.15f);
			double inicio = Math::Atan2(-dy, -dx);   // punto del lazo que queda dentro del nodo
			List<PointF>^ pts = gcnew List<PointF>();
			for (int k = 0; k <= 60; k++) {
				double ang = inicio + 2 * Math::PI * k / 60;
				PointF q((float)(o.X + r * Math::Cos(ang)), (float)(o.Y + r * Math::Sin(ang)));
				if (Distancia(q, c) >= R + (flecha ? 2 : 1)) pts->Add(q);
			}
			if (pts->Count < 2) return;
			Pen lapiz(col, ancho);
			if (flecha) PonerFlecha(%lapiz, ancho);
			g->DrawLines(%lapiz, pts->ToArray());
		}

		static float Distancia(PointF a, PointF b) {
			return (float)Math::Sqrt((a.X - b.X) * (a.X - b.X) + (a.Y - b.Y) * (a.Y - b.Y));
		}

		// DFS: nodos alcanzados desde el origen en este paso.
		static bool EsNuevo(Paso^ p, int k) {
			return p->tipo == TipoPaso::FilaDfs && p->filaTrabajo >= 0 && k != p->filaTrabajo && p->matriz[p->filaTrabajo, k] == 1;
		}

		static String^ OrdenTexto(array<int>^ orden) {
			String^ t = "";
			for (int k = 0; k < orden->Length; k++) t += (k > 0 ? ", " : "") + (orden[k] + 1).ToString();
			return t;
		}

		static void Encabezado(Graphics^ g, Rectangle c, String^ titulo, String^ sub) {
			TextoEn(g, titulo, Tema::Titulo, Tema::Texto, c.X + 14, c.Y + 10);
			if (!String::IsNullOrEmpty(sub)) {
				int w = (int)g->MeasureString(titulo, Tema::Titulo).Width;
				TextoEn(g, sub, Tema::Normal, Tema::TextoSuave, c.X + 22 + w, c.Y + 14);
			}
			Pen linea(Tema::Borde, 1.0f);
			g->DrawLine(%linea, c.X + 12, c.Y + 38, c.Right - 12, c.Y + 38);
		}

		static void Anillo(Graphics^ g, PointF c, float r, Color col, float ancho) {
			Pen p(col, ancho);
			g->DrawEllipse(%p, c.X - r, c.Y - r, 2 * r, 2 * r);
		}

		static int ItemLeyenda(Graphics^ g, int x, int y, Color col, String^ texto, bool anillo) {
			if (anillo) Anillo(g, PointF((float)x + 7, (float)y + 9), 6, col, 3.0f);
			else {
				SolidBrush b(col);
				g->FillRectangle(%b, x, y + 3, 14, 13);
				Pen p(Tema::Borde, 1.0f);
				g->DrawRectangle(%p, x, y + 3, 14, 13);
			}
			TextoEn(g, texto, Tema::Pequena, Tema::TextoSuave, x + 19, y + 1);
			return x + 19 + (int)g->MeasureString(texto, Tema::Pequena).Width + 16;
		}

		static void Celda(Graphics^ g, Rectangle rc, Color fondo, String^ t, Drawing::Font^ f, Color ct) {
			SolidBrush b(fondo);
			g->FillRectangle(%b, rc);
			Centrado(g, t, f, ct, rc);
		}

		static void Redondeado(Graphics^ g, Rectangle rc, int radio, Color col) {
			GraphicsPath gp;
			int d = Math::Min(radio * 2, Math::Min(rc.Width, rc.Height));
			gp.AddArc(rc.X, rc.Y, d, d, 180, 90);
			gp.AddArc(rc.Right - d, rc.Y, d, d, 270, 90);
			gp.AddArc(rc.Right - d, rc.Bottom - d, d, d, 0, 90);
			gp.AddArc(rc.X, rc.Bottom - d, d, d, 90, 90);
			gp.CloseFigure();
			SolidBrush b(col);
			g->FillPath(%b, %gp);
		}

		static void Centrado(Graphics^ g, String^ t, Drawing::Font^ f, Color col, Rectangle rc) {
			StringFormat sf;
			sf.Alignment = StringAlignment::Center;
			sf.LineAlignment = StringAlignment::Center;
			SolidBrush b(col);
			g->DrawString(t, f, %b, RectangleF((float)rc.X, (float)rc.Y, (float)rc.Width, (float)rc.Height), %sf);
		}

		static void TextoEn(Graphics^ g, String^ t, Drawing::Font^ f, Color col, int x, int y) {
			SolidBrush b(col);
			g->DrawString(t, f, %b, (float)x, (float)y);
		}
	};
}
