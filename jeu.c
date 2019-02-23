/*
	Canvas pour algorithmes de jeux à 2 joueurs
	
	joueur 0 : humain
	joueur 1 : ordinateur
			
*/
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#define S_COL 7
#define S_LIN 6

// Paramètres du jeu
#define LARGEUR_MAX S_COL 	// nb max de fils pour un noeud (= nb max de coups possibles)

#define TEMPS 5		// temps de calcul pour un coup avec MCTS (en secondes)

// macros
#define AUTRE_JOUEUR(i) (1-(i))
#define min(a, b)       ((a) < (b) ? (a) : (b))
#define max(a, b)       ((a) < (b) ? (b) : (a))

#define _c_ 1

// Critères de fin de partie
typedef enum {NON, MATCHNUL, ORDI_GAGNE, HUMAIN_GAGNE } FinDePartie;

// Definition du type Etat (état/position du jeu)
typedef struct EtatSt {

	int joueur; // à qui de jouer ? 

	char plateau[S_LIN][S_COL];	

} Etat;

// Definition du type Coup
typedef struct {

	int colonne;

} Coup;

// Copier un état 
Etat * copieEtat( Etat * src ) {
	Etat * etat = (Etat *)malloc(sizeof(Etat));

	etat->joueur = src->joueur;
	
	int l,c;	
	for (l=0; l< S_LIN; l++)
		for ( c=0; c<S_COL; c++)
			etat->plateau[l][c] = src->plateau[l][c];
	

	
	return etat;
}

// Etat initial 
Etat * etat_initial( void ) {
	Etat * etat = (Etat *)malloc(sizeof(Etat));
	
	int l,c;	
	for (l=0; l< S_LIN; l++)
		for ( c=0; c<S_COL; c++)
			etat->plateau[l][c] = ' ';
	
	return etat;
}


void afficheJeu(Etat * etat) {
	int l,c;
	printf(" |");
	for ( c = 0; c < S_COL; c++) 
		printf("%d|", c);
	printf("\n");
	printf("----------------");
	printf("\n");
	
	for(l=0; l < S_LIN; l++) {
		printf("%d|", l);
		for ( c = 0; c < S_COL; c++) 
			printf("%c|", etat->plateau[l][c]);
		printf("\n");
	}
	printf("----------------");
	printf("\n");
	fflush(stdout);
}


// Nouveau coup 
Coup * nouveauCoup( int c ) {
	Coup * coup = (Coup *)malloc(sizeof(Coup));

	coup->colonne = c;
	
	return coup;
}

// Demander à l'humain quel coup jouer 
Coup * demanderCoup () {
	int c;
	printf(" quelle colonne ? ") ;
	scanf("%d",&c); 
	
	return nouveauCoup(c);
}

// Modifier l'état en jouant un coup 
// retourne 0 si le coup n'est pas possible
int jouerCoup( Etat * etat, Coup * coup ) {

	int ligne;
	int peux_jouer = 0;
	for(ligne = S_LIN-1; ligne >= 0; ligne--){
		if(etat->plateau[ligne][coup->colonne] == ' '){
			peux_jouer++;
			break;
		}
	}
	if(!peux_jouer)
		return 0;

	etat->plateau[ligne][coup->colonne] = etat->joueur ? 'O' : 'X';
	
	// à l'autre joueur de jouer
	etat->joueur = AUTRE_JOUEUR(etat->joueur); 	

	return 1;
}

// Retourne une liste de coups possibles à partir d'un etat 
// (tableau de pointeurs de coups se terminant par NULL)
Coup ** coups_possibles( Etat * etat ) {
	
	Coup ** coups = (Coup **) malloc((1+LARGEUR_MAX) * sizeof(Coup *) );
	
	int k = 0;

	int c;
	for (c=0; c < S_COL && k < LARGEUR_MAX; c++) {
		if(etat->plateau[0][c] == ' '){
			coups[k] = nouveauCoup(c); 
			k++;
		}
	}
	
	coups[k] = NULL;

	return coups;
}


// Definition du type Noeud 
typedef struct NoeudSt {
		
	int joueur; // joueur qui a joué pour arriver ici
	Coup * coup;   // coup joué par ce joueur pour arriver ici
	
	Etat * etat; // etat du jeu
			
	struct NoeudSt * parent; 
	struct NoeudSt * enfants[LARGEUR_MAX]; // liste d'enfants : chaque enfant correspond à un coup possible
	int nb_enfants;	// nb d'enfants présents dans la liste
	
	// POUR MCTS:
	int nb_victoires;
	int nb_simus;
	
} Noeud;


// Créer un nouveau noeud en jouant un coup à partir d'un parent 
// utiliser nouveauNoeud(NULL, NULL) pour créer la racine
Noeud * noeudRacine (Etat * etat) {
	Noeud * noeud = (Noeud *)malloc(sizeof(Noeud));
	
	noeud->etat = copieEtat (etat);
	noeud->coup = NULL;			
	noeud->joueur = 1;		

	
	noeud->parent = NULL;
	noeud->nb_enfants = 0; 
	
	// POUR MCTS:
	noeud->nb_victoires = 0;
	noeud->nb_simus = 0;	
	

	return noeud; 	
}


// Créer un nouveau noeud en jouant un coup à partir d'un parent 
// utiliser nouveauNoeud(NULL, NULL) pour créer la racine
Noeud * nouveauNoeud (Noeud * parent, Coup * coup ) {
	Noeud * noeud = (Noeud *)malloc(sizeof(Noeud));
	

	//A aucun moment on aura besoin de creer un noeud sans parent ou sans coup 
	// (exception creer racine) 
	//Donc si on laisse la possibilite de creer un noeud sans parent ou sans coup 
	// On ne fait que reporter le problème 
	//Arreter ici le programme simplifiera le débuggage
	if(parent == NULL || coup == NULL){
		printf("\nCreer noeud avec un parent ou un coup null\n");
		exit(1);
	}

	noeud->etat = copieEtat ( parent->etat );
	jouerCoup ( noeud->etat, coup );
	noeud->coup = coup;			
	noeud->joueur = AUTRE_JOUEUR(parent->joueur);		

	
	noeud->parent = parent; 
	noeud->nb_enfants = 0; 
	
	// POUR MCTS:
	noeud->nb_victoires = 0;
	noeud->nb_simus = 0;	
	

	return noeud; 	
}

// Ajouter un enfant à un parent en jouant un coup
// retourne le pointeur sur l'enfant ajouté
Noeud * ajouterEnfant(Noeud * parent, Coup * coup) {
	Noeud * enfant = nouveauNoeud (parent, coup ) ;
	parent->enfants[parent->nb_enfants] = enfant;
	parent->nb_enfants++;
	return enfant;
}

void freeNoeud ( Noeud * noeud) {
	if ( noeud->etat != NULL)
		free (noeud->etat);
		
	while ( noeud->nb_enfants > 0 ) {
		freeNoeud(noeud->enfants[noeud->nb_enfants-1]);
		noeud->nb_enfants --;
	}
	if ( noeud->coup != NULL)
		free(noeud->coup); 

	free(noeud);
}
	

// Test si l'état est un état terminal 
// et retourne NON, MATCHNUL, ORDI_GAGNE ou HUMAIN_GAGNE
FinDePartie testFin( Etat * etat ) {
	
	// tester si un joueur a gagné
	int l,c,k,n = 0;
	for ( l=0;l < S_LIN; l++) {
		for(c=0; c < S_COL; c++) {
			if ( etat->plateau[l][c] != ' ') {
				n++;	// nb coups joués
			
				//Colonne (la ligne change)
				k=0;
				while ( k < 4 && l+k < S_LIN && etat->plateau[l+k][c] == etat->plateau[l][c] ) 
					k++;
				if ( k == 4 ) 
					return etat->plateau[l][c] == 'O'? ORDI_GAGNE : HUMAIN_GAGNE;

				//Ligne (la colonne change)
				k=0;
				while ( k < 4 && c+k < S_COL && etat->plateau[l][c+k] == etat->plateau[l][c] ) 
					k++;
				if ( k == 4 ) 
					return etat->plateau[l][c] == 'O'? ORDI_GAGNE : HUMAIN_GAGNE;

				// diagonales
				k=0;
				while ( k < 4 && l+k < S_LIN && c+k < S_COL && etat->plateau[l+k][c+k] == etat->plateau[l][c] ) 
					k++;
				if ( k == 4 ) 
					return etat->plateau[l][c] == 'O'? ORDI_GAGNE : HUMAIN_GAGNE;

				k=0;
				while ( k < 4 && l+k < S_LIN && c-k >= 0 && etat->plateau[l+k][c-k] == etat->plateau[l][c] ) 
					k++;
				if ( k == 4 ) 
					return etat->plateau[l][c] == 'O'? ORDI_GAGNE : HUMAIN_GAGNE;		
			}
		}
	}

	// et sinon tester le match nul	
	if ( n == S_LIN*S_COL ) 
		return MATCHNUL;
		
	return NON;
}



// Calcule et joue un coup de l'ordinateur avec MCTS-UCT
// en tempsmax secondes
void ordijoue_mcts(Etat * etat, int tempsmax) {
	clock_t tic, toc;
	tic = clock();
	int temps;

	Coup * meilleur_coup ;
	
	// Créer l'arbre de recherche
	Noeud * racine = noeudRacine(etat);	
	int iter = 0;
	
	do {
	
	
		//Tant qu'on a le temps et qu'on a pas fini d'explorer la branche choisie
		Noeud * noeudActuel = racine;
		FinDePartie fin_de_partie = testFin(noeudActuel->etat);


		while(fin_de_partie == NON){
			noeudActuel->nb_simus++;
			if(noeudActuel->nb_enfants == 0){
				Coup ** coup_possible = coups_possibles(noeudActuel->etat);
				int nbCoup = 0;
				while(coup_possible[nbCoup] != NULL){
					Coup* _fils = coup_possible[nbCoup];
					ajouterEnfant(noeudActuel, _fils);
					nbCoup++;
				}
			}

			int i = 0; 
			int max_i = 0;
			float meilleur_B = -500;
			for(; i < noeudActuel->nb_enfants; i++){
				if(noeudActuel->enfants[i]->nb_simus == 0){
					max_i = i;
					break;
				}

				Noeud* nenfant = noeudActuel->enfants[i];

				float mu = (float)nenfant->nb_victoires / (float)nenfant->nb_simus;
				
				if(noeudActuel->joueur == 0) //humain
					mu = -mu;
				
				float racine_bizarre = ((float)noeudActuel->nb_simus / (float)nenfant->nb_simus);
				
				float actual_B = mu + _c_ *  racine_bizarre;
				
				if(actual_B > meilleur_B){
					max_i = i;
					meilleur_B = actual_B;
				}
			}
			
			noeudActuel = noeudActuel->enfants[max_i];
			fin_de_partie = testFin(noeudActuel->etat);
		}

		if(fin_de_partie == ORDI_GAGNE){
			while(noeudActuel != NULL){
				noeudActuel->nb_victoires++;
				noeudActuel = noeudActuel->parent;
			}
		}

		toc = clock(); 
		temps = (int)( ((double) (toc - tic)) / CLOCKS_PER_SEC );
		iter ++;
	} while ( temps < tempsmax );


	printf("\n#####################################\n");
	printf("#####################################\n");

	int j = 0;
	int meuilleur_j = 0;
	int s,v,d;
	int meilleur_s_j = 0;
	printf("Nb simu: %d\n", iter);
	for (; j < racine->nb_enfants; j++){
		s = racine->enfants[j]->nb_simus;
		v = racine->enfants[j]->nb_victoires;
		d = s-v;
		printf("Enfant %5d. Nb Simu: %5d\t\tNb Victoire: %5d\t\tNb defaite: %5d\n", j, s, v, d);
		if(meilleur_s_j < s){
			meuilleur_j = j;
			meilleur_s_j = s;
		}
	}

	s = racine->nb_simus; // normalement = à iter
	v = racine->nb_victoires;
	d = s-v;
	printf("total:  Nb Simu: %5d\t\tNb Victoire: %5d\t\tNb defaite: %5d\n", j, s, v, d);
	printf("#####################################\n");
	printf("#####################################\n");

	
	meilleur_coup = racine->enfants[meuilleur_j]->coup;
	/* fin de l'algorithme  */ 
	
	// Jouer le meilleur premier coup
	jouerCoup(etat, meilleur_coup );
	
	// Penser à libérer la mémoire :
	freeNoeud(racine);
}

int main(void) {

	Coup * coup;
	FinDePartie fin;
	
	// initialisation
	Etat * etat = etat_initial(); 
	
	// Choisir qui commence : 
	printf("Qui commence (0 : humain, 1 : ordinateur) ? ");
	scanf("%d", &(etat->joueur) );
	
	// boucle de jeu
	do {
		printf("\n");
		afficheJeu(etat);
		
		if ( etat->joueur == 0 ) {
			// tour de l'humain
			
			do {
				coup = demanderCoup();
			} while ( !jouerCoup(etat, coup) );
									
		}
		else {
			// tour de l'Ordinateur
			
			ordijoue_mcts( etat, TEMPS );
			
		}
		
		fin = testFin( etat );
	}	while ( fin == NON ) ;

	printf("\n");
	afficheJeu(etat);
		
	if ( fin == ORDI_GAGNE )
		printf( "** L'ordinateur a gagné **\n");
	else if ( fin == MATCHNUL )
		printf(" Match nul !  \n");
	else
		printf( "** BRAVO, l'ordinateur a perdu  **\n");
	return 0;
}
