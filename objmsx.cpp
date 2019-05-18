#include "objmsx.h"

BufferMSX::BufferMSX(int width, int height) {
	this->width = width;
	this->height = height;
	// Set up sizes. (HEIGHT x WIDTH)
	buffer.resize(this->width);
	for (int x = 0; x < this->width; x++) {
		buffer[x].resize(height);
	}
}

void BufferMSX::setMSX(int x, int y, int cormsx) {
	buffer[x][y] = cormsx;
}

void BufferMSX::getRGB(int x, int y, double &R, double &G, double &B) {
	R = (double) MSX_Colors1[buffer[x][y]][0];
	G = (double) MSX_Colors1[buffer[x][y]][1];
	B = (double) MSX_Colors1[buffer[x][y]][2];
}

Bloco::Bloco(int width, int height, double limit) {
	this->width = width;
	this->height = height;
	this->limit = limit;
	// Set up sizes. (HEIGHT x WIDTH)
	bufferORIG.resize(this->width);
	bufferNOVO.resize(this->width);
	bufferMSXORIG.resize(this->width);
	bufferMSXNOVO.resize(this->width);
	for (int x = 0; x < this->width; x++) {
		bufferORIG[x].resize(this->height);
		bufferNOVO[x].resize(this->height);
		bufferMSXORIG[x].resize(this->height);
		bufferMSXNOVO[x].resize(this->height);
		for (int y = 0; y < this->height; y++) {
			bufferORIG[x][y].resize(4);
			bufferNOVO[x][y].resize(4);
		}
	}
}

void Bloco::setORIG(int x, int y, double R, double G, double B, double pos) {
	bufferORIG[x][y][0] = R;
	bufferORIG[x][y][1] = G;
	bufferORIG[x][y][2] = B;
	bufferORIG[x][y][3] = pos;
}

void Bloco::ProcessaBloco(std::vector< CorPossivel > &corespossiveis) {
	this->corespossiveis = &corespossiveis;
	std::map< int, InfoCores > mapacores;
	CopiaNovo();
	AchaCoresMaisProximas(false);
	ContaCores(mapacores);
	bool refaz = mapacores.size() >= 3;
	while (refaz) {
		AchaCoresMixar(mapacores);
		MixaCores(mapacores);
		AchaCoresMaisProximas(false);
		ContaCores(mapacores);
		refaz = mapacores.size() >= 3;
	}
	CopiaNovoMSX();
	//Tenta dither
	AchaCoresMaisProximas(true);
	ContaCores(mapacores);
	if (mapacores.size() <= 2) {
		CopiaNovoMSX();
	}
}

void Bloco::AchaCoresMaisProximas(bool comdither) {
	for (int x = 0; x < this->width; x++) {
		for (int y = 0; y < this->height; y++) {
			int cor1, cor2;
			double proporcao;
			double R, G, B, pos;
			R = bufferNOVO[x][y][0];
			G = bufferNOVO[x][y][1];
			B = bufferNOVO[x][y][2];
			pos = bufferNOVO[x][y][3];
			
			ColorSpace::Rgb c(R, G, B);
			bool primeiracor = true;
			double distmenor;
			
			if (comdither) {
				for (std::vector<CorPossivel>::iterator coratual = (*corespossiveis).begin() ; coratual != (*corespossiveis).end(); coratual++) {
					ColorSpace::Rgb d((*coratual).R, (*coratual).G, (*coratual).B);
					double distatual = ColorSpace::Cie2000Comparison::Compare(&c, &d);
					bool trocacor = false;
					if (primeiracor) {
						trocacor = true;
						primeiracor = false;
					} else if (distatual < distmenor) {
						trocacor = true;
					}
					if (trocacor) {
						distmenor = distatual;
						cor1 = (*coratual).cor1;
						cor2 = (*coratual).cor2;
						
						//Calcula proporcao
						double NY1;
						NY1 = MSX_Colors1[cor1][3];

						double NY2;
						NY2 = MSX_Colors1[cor2][3];
					
						if (NY1 == NY2) { //in case of 0 length line
							proporcao = 0.5;
						} else {
							double NY = 0.299 * R + 0.587 * G + 0.114 * B;
							proporcao = (NY - NY1) / (NY2 - NY1);
							if (proporcao <= 0.0) {
								proporcao = 0.0;
							}
							else if (proporcao >= 1.0) {
								proporcao = 1.0;
							}
						}
					};
				}
				
				int cormsx = (proporcao * this->limit) < pos ? cor1 : cor2;
				bufferMSXORIG[x][y] = cormsx;
				
			} else {
				for (int coratual = 1; coratual < MSXNCOLORS; coratual++) {
					ColorSpace::Rgb d(MSX_Colors1[coratual][0], MSX_Colors1[coratual][1], MSX_Colors1[coratual][2]);
					double distatual = ColorSpace::Cie2000Comparison::Compare(&c, &d);
					bool trocacor = false;
					if (primeiracor) {
						trocacor = true;
						primeiracor = false;
					} else if (distatual < distmenor) {
						trocacor = true;
					}
					if (trocacor) {
						distmenor = distatual;
						cor1 = coratual;
					};
				}
				
				bufferMSXORIG[x][y] = cor1;
			}
			
		}
	}
}

void Bloco::getMSX(int x, int y, int &cormsx) {
	cormsx = bufferMSXNOVO[x][y];
}

void Bloco::ContaCores(std::map< int, InfoCores > &mapacores) {
	mapacores.clear();
	for (int x = 0; x < this->width; x++) {
		for (int y = 0; y < this->height; y++) {
			std::pair< int, int > ponto = std::make_pair(x, y);
			int cormsx = bufferMSXORIG[x][y];
			
			// Searching element in std::map by key.
			std::map< int, InfoCores >::iterator coratual = mapacores.find(cormsx);
			
			if(coratual == mapacores.end()) {
				InfoCores  infocor;
				infocor.quantidade = 1;
				infocor.pontos.push_back(ponto);
				mapacores.insert(std::make_pair(cormsx, infocor));
			} else {
				(*coratual).second.quantidade++;
				(*coratual).second.pontos.push_back(ponto);
			}
		}
	}
}

void Bloco::CopiaNovo() {
	for (int x = 0; x < this->width; x++) {
		for (int y = 0; y < this->height; y++) {
			bufferNOVO[x][y][0] = bufferORIG[x][y][0];
			bufferNOVO[x][y][1] = bufferORIG[x][y][1];
			bufferNOVO[x][y][2] = bufferORIG[x][y][2];
			bufferNOVO[x][y][3] = bufferORIG[x][y][3];
		}
	}
}

void Bloco::AchaCoresMixar(std::map< int, InfoCores > &mapacores) {
	double R, G, B;
	for (std::map< int, InfoCores >::iterator grupoatual = mapacores.begin() ; grupoatual != mapacores.end(); grupoatual++) {
		R = 0.0;
		G = 0.0;
		B = 0.0;
		
		//Mixa cores
		for (std::vector< std::pair< int, int > >::iterator pontoatual = (*grupoatual).second.pontos.begin() ; pontoatual != (*grupoatual).second.pontos.end(); pontoatual++) {
			R = R + pow(bufferORIG[(*pontoatual).first][(*pontoatual).second][0], 2.0);
			G = G + pow(bufferORIG[(*pontoatual).first][(*pontoatual).second][1], 2.0);
			B = B + pow(bufferORIG[(*pontoatual).first][(*pontoatual).second][2], 2.0);
		}
		
		R = sqrt(R / (*grupoatual).second.quantidade);
		G = sqrt(G / (*grupoatual).second.quantidade);
		B = sqrt(B / (*grupoatual).second.quantidade);
		
		(*grupoatual).second.R = R;
		(*grupoatual).second.G = G;
		(*grupoatual).second.B = B;
	}
	
	//Calcula distancia
	double R1, G1, B1, R2, G2, B2;
	bool primeiracor = true;
	double distmenor;
	std::map< int, InfoCores >::iterator grupo1;
	std::map< int, InfoCores >::iterator grupo2;
	
	std::map< int, InfoCores >::iterator grupoatual1 = mapacores.begin();
	std::map< int, InfoCores >::iterator grupoatual2 = grupoatual1;
	grupoatual2++;
	bool acabou = false;

	while (!acabou) {
		R1 = (*grupoatual1).second.R;
		G1 = (*grupoatual1).second.G;
		B1 = (*grupoatual1).second.B;
		R2 = (*grupoatual2).second.R;
		G2 = (*grupoatual2).second.G;
		B2 = (*grupoatual2).second.B;
		
		ColorSpace::Rgb c(R1, G1, B1);
		ColorSpace::Rgb d(R2, G2, B2);
		double distatual = ColorSpace::Cie2000Comparison::Compare(&c, &d);
		
		bool trocacor = false;
		if (primeiracor) {
			trocacor = true;
			primeiracor = false;
		} else if (distatual < distmenor) {
			trocacor = true;
		}
		if (trocacor) {
			distmenor = distatual;
			grupo1 = grupoatual1;
			grupo2 = grupoatual2;
		};

		grupoatual2++;
		//Passei do fim
		if (grupoatual2 == mapacores.end()) {
			//Próximo grupo
			grupoatual1++;
			grupoatual2 = grupoatual1;
			grupoatual2++;
			//É o último
			if (grupoatual2 == mapacores.end()) {
				acabou = true;
			}
		}
	}
	(*grupo1).second.pontos.insert( (*grupo1).second.pontos.end(), (*grupo2).second.pontos.begin(), (*grupo2).second.pontos.end() );
	(*grupo1).second.quantidade = (*grupo1).second.quantidade + (*grupo2).second.quantidade;
	int chave = (*grupo2).first;
	mapacores.erase(chave);
}

void Bloco::MixaCores(std::map< int, InfoCores > &mapacores) {
	double R, G, B;
	for (std::map< int, InfoCores >::iterator grupoatual = mapacores.begin() ; grupoatual != mapacores.end(); grupoatual++) {
		R = 0.0;
		G = 0.0;
		B = 0.0;
		
		//Mixa cor
		for (std::vector< std::pair< int, int > >::iterator pontoatual = (*grupoatual).second.pontos.begin() ; pontoatual != (*grupoatual).second.pontos.end(); pontoatual++) {
			R = R + pow(bufferORIG[(*pontoatual).first][(*pontoatual).second][0], 2.0);
			G = G + pow(bufferORIG[(*pontoatual).first][(*pontoatual).second][1], 2.0);
			B = B + pow(bufferORIG[(*pontoatual).first][(*pontoatual).second][2], 2.0);
		}
		
		R = sqrt(R / (*grupoatual).second.quantidade);
		G = sqrt(G / (*grupoatual).second.quantidade);
		B = sqrt(B / (*grupoatual).second.quantidade);
			
		//Copia cor
		for (std::vector< std::pair< int, int > >::iterator pontoatual = (*grupoatual).second.pontos.begin() ; pontoatual != (*grupoatual).second.pontos.end(); pontoatual++) {
			bufferNOVO[(*pontoatual).first][(*pontoatual).second][0] = R;
			bufferNOVO[(*pontoatual).first][(*pontoatual).second][1] = G;
			bufferNOVO[(*pontoatual).first][(*pontoatual).second][2] = B;
		}
	}
}

void Bloco::CopiaNovoMSX() {
	for (int x = 0; x < this->width; x++) {
		for (int y = 0; y < this->height; y++) {
			bufferMSXNOVO[x][y] = bufferMSXORIG[x][y];
		}
	}
}

void MSXConvert::Convert(BufferRGB &bufferin, BufferRGB &bufferout, ObjGimp &objectgimp) {
	PreencheCoresPossiveis();
	int progresso = 0;
	int width, height;
	bufferin.getSize(width, height);
	
	//Cria msx
	BufferMSX buffermsx(width, height);

	chunksH = width / WIDTHATTR + 1;
	chunksV = height / HEIGHTATTR + 1;

	chunkV = 0;
	bool acabouV = false;
	//Para cada grupo vertical
	while (not acabouV) {
		chunkH = 0;
		bool acabouH = false;
		//Para cada grupo horizontal
		while (not acabouH) {
			
			Bloco bloco(WIDTHATTR, HEIGHTATTR, (double) MSXTDITHER * MSXTDITHER);
			//Preenche Bloco
			for (int y = 0; y <= (HEIGHTATTR - 1); y++) {
				int j = chunkV * HEIGHTATTR + y;
				for (int x = 0; x <= (WIDTHATTR - 1); x++) {
					int i = chunkH * WIDTHATTR + x;
					if ((i < width) && (j < height)) {
						double R, G, B;
						bufferin.getRGB(i, j, R, G, B);
						int map_value;
						if (MSXTDITHER == 2) {
							map_value = map2[((j % 2) * 2) + (i % 2)];
						} else if (MSXTDITHER == 4) {
							map_value = map4[((j % 4) * 4) + (i % 4)];
						} else if (MSXTDITHER == 8) {
							map_value = map8[((j % 8) * 8) + (i % 8)];
						}
						bloco.setORIG(x, y, R, G, B, map_value);
					}
				}
			}
			
			//Acha cores com dither
			bloco.ProcessaBloco(corespossiveis);
			
			//Preenche MSX
			for (int y = 0; y <= (HEIGHTATTR - 1); y++) {
				int j = chunkV * HEIGHTATTR + y;
				for (int x = 0; x <= (WIDTHATTR - 1); x++) {
					int i = chunkH * WIDTHATTR + x;
					if ((i < width) && (j < height)) {
						int cormsx;
						bloco.getMSX(x, y, cormsx);
						buffermsx.setMSX(i, j, cormsx);
					}
				}
			}

			//Atualiza Progresso
			progresso++;
			objectgimp.AtualizaProgresso((double)progresso / (double)(chunksV * chunksH));
			
			//atualiza coluna
			chunkH++;
			if (chunkH >= chunksH) {
				acabouH = true;
			};
		};
		//atualiza linha
		chunkV++;
		if (chunkV >= chunksV) {
			acabouV = true;
		};
	};

	//Copia buffermsx em bufferout
	for (int j = 0; j < height; j++) {
		for (int i = 0; i < width; i++) {
			double R, G, B;
			buffermsx.getRGB(i, j, R, G, B);
			bufferout.setRGB(i, j, R, G, B);
		}
	}	
}

void MSXConvert::PreencheCoresPossiveis() {
	//Processa grupos
	for (int grupoatual1 = 0; grupoatual1 < MSXNGRUPOS; grupoatual1++) {
		for (int grupoatual2 = 0; grupoatual2 < MSXNGRUPOS; grupoatual2++) {
			bool processar = false;
			//Dentro do grupo
			if (grupoatual1 == grupoatual2) {
				processar = true;
			}
			//Sem cores x cores
			else
			if (grupoatual1 == 0) {
				processar = true;
			}
			//Entre grupos
			else
			if ((grupoatual2 == (grupoatual1 +1)) || (grupoatual1 == MSXNGRUPOS) &&  (grupoatual2 == 1)) {
				processar = true;
			}
			if (processar) {
				int coratual1 = 0;
				int coratual2 = 0;
				bool temnogrupo = true;
				while (temnogrupo) {
					int cor1 = MSX_Groups[grupoatual1][coratual1];
					int cor2 = MSX_Groups[grupoatual2][coratual2];
					if ((cor1 != 0) && (cor2 != 0)) {
						int NY1 = MSX_Colors1[cor1][3];
						int NY2 = MSX_Colors1[cor2][3];
						if (std::abs(NY2 - NY1) <= MSXDMANUAIS) {
							ArmazenaCor(cor1, cor2);
						}
					}
					bool podeavancar1 = true;
					bool podeavancar2 = true;
					if (coratual1 == 2) {
						podeavancar1 = false;
					}
					if (coratual2 == 2) {
						podeavancar2 = false;
					}
					int coratual3 = coratual1 + 1;
					int coratual4 = coratual2 + 1;
					int cor3 = MSX_Groups[grupoatual1][coratual1];
					if (cor3 == 0) {
						podeavancar1 = false;
					}
					int cor4 = MSX_Groups[grupoatual2][coratual2];
					if (cor4 == 0) {
						podeavancar2 = false;
					}
					if (!podeavancar1 && !podeavancar2) {
						temnogrupo = false;
					} else if (podeavancar1 && !podeavancar2) {
						coratual1++;
					} else if (!podeavancar1 && podeavancar2) {
						coratual2++;
					} else {
						int NY3 = MSX_Colors1[cor3][3];
						int NY4 = MSX_Colors1[cor4][3];
						if (NY3 <= NY4) {
							coratual1++;
						} else {
							coratual2++;
						}
					}
				}
			}
		}
	}
}

void MSXConvert::ArmazenaCor(int cor1, int cor2) {
	double NR1, NG1, NB1, NY1;
	NR1 = MSX_Colors1[cor1][0];
	NG1 = MSX_Colors1[cor1][1];
	NB1 = MSX_Colors1[cor1][2];
	NY1 = MSX_Colors1[cor1][3];
	
	CorPossivel corpossivel;
	if (cor1 == cor2) {
		corpossivel.R = NR1;
		corpossivel.G = NG1;
		corpossivel.B = NB1;
		corpossivel.cor1 = cor1;
		corpossivel.cor2 = cor2;
	} else {
		double NR2, NG2, NB2, NY2;
		NR2 = MSX_Colors1[cor2][0];
		NG2 = MSX_Colors1[cor2][1];
		NB2 = MSX_Colors1[cor2][2];
		NY2 = MSX_Colors1[cor2][3];
		
		//Preenche
		corpossivel.R = sqrt((pow(NR1, 2.0) + pow(NR2, 2.0)) / 2);
		corpossivel.G = sqrt((pow(NG1, 2.0) + pow(NG2, 2.0)) / 2);
		corpossivel.B = sqrt((pow(NB1, 2.0) + pow(NB2, 2.0)) / 2);
		if (NY1 <= NY2) {
			corpossivel.cor1 = cor1;
			corpossivel.cor2 = cor2;
		} else {
			corpossivel.cor1 = cor2;
			corpossivel.cor2 = cor1;
		}
	}
	//Armazena
	corespossiveis.push_back(corpossivel);
}
