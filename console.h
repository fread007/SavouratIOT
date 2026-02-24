/*
 * Fichier d'en-tête pour la gestion de la console.
 * 
 * Taille du terminal :
 *   - les lignes sont horizontales
 *   - les colonnes sont verticales
 * 
 * *Nota Bene :* vous devez choisir des valeurs pour ncols et nrows
 * qui correspondent à la taille de votre fenêtre de terminal Linux. 
 * Vous pouvez généralement le voir dans les menus de la fenêtre. 
 * Une taille typique est 80x24 ou 80x43 au format (ncols,nrows).
 * 
 * Important : certaines émulations de terminal appliquent une politique 
 * de retour à la ligne à la fin d'une ligne plutôt que de bloquer le curseur. 
 * Dans ce cas, nous vous suggérons d'utiliser une valeur plus petite pour 
 * ncols, comme 79 au lieu de 80, pour éviter cette politique de retour à la ligne.
 * 
 * Couleurs :
 * - COLOR_RESET : réinitialise les couleurs d'encre et de fond.
 * - Couleurs d'encre (premier plan) : BLACK, RED, GREEN, YELLOW, BLUE, 
 *   MAGENTA, CYAN, WHITE.
 * - Couleurs de fond : BG_BLACK, BG_RED, BG_GREEN, BG_YELLOW, BG_BLUE, 
 *   BG_MAGENTA, BG_CYAN, BG_WHITE.
 * 
 * Fonctions :
 * - Mouvement du curseur : cursor_left(), cursor_right(), cursor_down(), 
 *   cursor_up().
 * - Déplacement du curseur à des coordonnées données : cursor_at(int row, int col).
 * - Obtention de la position actuelle du curseur : cursor_position(int* row, int* col).
 * - Masquer/afficher le curseur : cursor_hide(), cursor_show().
 * - Changer la couleur : console_color(uint8_t color).
 * - Effacer le terminal : console_clear().
 * - Initialiser la console avec un rappel pour chaque ligne entrée : 
 *   console_init(void (*callback)(char*)).
 * - Écho des caractères ASCII et gestion des caractères spéciaux : 
 *   console_echo(uint8_t byte).
 */
#ifndef _CONSOLE_H_
#define _CONSOLE_H_

/*
 * Terminal Size: 
 *   - rows are horizontal
 *   - columns are vertical
 * 
 * *Nota Bene:* you must choose values for the ncols and nrows
 * that match your Linux terminal window size. You can normally
 * see that in the window's menus. A typical size is 80x24 or
 * 80x43 in the (ncols,nrows) format.
 * 
 * Important: some terminal emulation emulate a wrap-around policy 
 * at the end of a line rather than blocking the cursor. 
 * If it is the case, we suggest that you use a smaller
 * ncols, like 79 instead of 80, to avoid this wrapping-around policy.
 */
#define NCOLS 80
#define NROWS 24

// the reset color resets both the ink and background colors.
#define COLOR_RESET 0

// the following colors are for the ink (foreground)
#define BLACK 30
#define RED 31
#define GREEN 32
#define YELLOW 33
#define BLUE 34
#define MAGENTA 35
#define CYAN 36
#define WHITE 37

// the following colors are for the background
#define BG_BLACK (BLACK+10)
#define BG_RED (RED+10)
#define BG_GREEN (GREEN+10)
#define BG_YELLOW (YELLOW+10)
#define BG_BLUE (BLUE+10)
#define BG_MAGENTA (MAGENTA+10)
#define BG_CYAN (CYAN+10)
#define BG_WHITE (WHITE+10)


#define TIMER0 0x101E2000

/*
 * Functions to move the cursor from its current position
 */
void cursor_left();
void cursor_right();
void cursor_down();
void cursor_up();

/*
 * Function to move the cursor to the given coordinates
 */ 
void cursor_at(int row, int col);

/*
 * Functions to obtain the current cursor position 
 */
void cursor_position(int* row, int* col);

/* 
 * Functions to hide/show the terminal cursor
 */
void cursor_hide();
void cursor_show();

/*
 * Function to set the color, either for the ink or background
 */
void console_color(uint8_t color);

/*
 * Clears the terminal, like the bash command `clear`.
 * Positions the cursor at (0,0).
 */
void console_clear();

/*
 * Initializes the console, giving the callback
 * to call for each line entered on the keyboard.
 * A line is a C string but contains only ASCII 
 * characters ([32-126]), as a C string it is 
 * terminated by a '\0'.
 * A line is validated by the end user by hitting 
 * the key `Enter`.
 */
void console_init(void (*callback)(char*));

/*
 * Call this function with every byte read from the "keyboard".
 * Echoes to the terminal only ASCII characters ([32-126]).
 * Recognized special characters:
 *   - arrow keys (left,right,up,down)
 *   - delete key
 *   - backspace (code 127 or 8)
 *   - ctrl-c to clear the terminal
 */
void console_echo(uint8_t byte);

#endif /* _CONSOLE_H_ */