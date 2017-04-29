/*
 * gfixer.c
 *
 * fix a gerber file which has multiple plots to the same
 * location, caused by plotting 3 power planes on top of 
 * each other.
 *
 * read in a gerber file (RS274X), sort all the data by X,Y,D
 * remove plots to the same location, keeping only the last.
 *
 * This counts on the fact that eagle creates macros for thermals
 * with D codes higher than the D code it uses for a via.
 *
 * So, if you sort by X,Y,D-code the thermal will be last.
 * (if I were smarter I'd figure out which D-code were the thermals)
 *
 * brad@heeltoe.com 9/2006
 */

#include <stdio.h>
#include <stdlib.h>

struct pad_s {
	int d;
	int x, y;
	int pen;
	int line;
	char *text;
} pads[5000];
int npads;

char *lines[5000];

int
g_parse_parameters(char *line)
{
	int parm;

#define TC(a, b) (((a) << 8) | (b))

	parm = (line[1] << 8) | line[2];
	switch (parm) {
	case TC('O', 'F'):
		break;
	case TC('F', 'S'):
		break;
	case TC('I', 'P'):
		break;
	case TC('L', 'P'):
		break;
	case TC('A', 'M'):
		break;
	case TC('A', 'D'):
		break;
	}

	return 0;
}

/* compare two pads, based on x,y & D code */
int compare(const void *p1, const void *p2)
{
	struct pad_s *pp1, *pp2;

	pp1 = (struct pad_s *)p1;
	pp2 = (struct pad_s *)p2;

	if (pp1->x != pp2->x)
		return pp1->x - pp2->x;

	if (pp1->y != pp2->y)
		return pp1->y - pp2->y;

	return pp1->d - pp2->d;
}

/* */
int
getnum(char *p, int left, int right, int *pnum)
{
	*pnum = 0;
	*pnum = atoi(p);
	return 0;
}

/* parse line of form "X106026Y050665D01*" */
int
g_parse_coord(char *line, int *px, int *py, int *pd)
{
	int len;
	char *cx, *cy, *cd;

	len = strlen(line);
	if (len && line[len-1] == '\n')
		len--;

	if (line[len-1] != '*') {
		fprintf(stderr, "coord data not terminated: %s", line);
		return -1;
	}

	cx = line;
	if (*cx != 'X') {
		return -1;
	}
	cx++;

	cy = strchr(cx, 'Y');
	if (cy == NULL)
		return -1;
	cy++;

	cd = strchr(cy, 'D');
	if (cd == NULL)
		return -1;
	cd++;

	if (getnum(cx, 6, 0, px))
		return -1;
	if (getnum(cy, 2, 0, py))
		return -1;
	if (getnum(cd, 2, 0, pd))
		return -1;

	return 0;
}

int
parse_gerber_file(FILE *f)
{
	char line[256], *text;
	int i, j, n, d, lineno;

	lineno = 1;
	while (fgets(line, sizeof(line), f)) {

		int x, y, pen;

		text = malloc(strlen(line)+1);
		strcpy(text, line);
		lines[lineno] = text;

		switch (line[0]) {
		case '%':
			g_parse_parameters(line);
			break;
		case 'G':
			break;
		case 'D':
			if (line[0] == 'D' && line[3] == '*') {
				d = atoi(&line[1]);
				if (0) printf("D%d\n", d);
			}
			break;

		case 'M':
			break;

		case 'X':
			if (d) {
#if 0
				char xx[7], yy[7];
				memcpy(xx, &line[1], 6);
				xx[6] = 0;
				memcpy(yy, &line[8], 6);
				yy[6] = 0;
				x = atoi(xx);
				y = atoi(yy);
				pen = 0;
#else
				if (g_parse_coord(line, &x, &y, &pen)) {
					break;
				}
#endif
				pads[npads].x = x;
				pads[npads].y = y;
				pads[npads].pen = pen;
				pads[npads].d = d;
				pads[npads].line = lineno;
				npads++;
			}
			break;
		}

		lineno++;
	}

	if (0) printf("npads %d\n", npads);
	qsort((void *)pads, npads, sizeof(struct pad_s), compare);

	if (0) {
		for (i = 0; i < 10; i++) {
			printf("D%02d %06d %06d\n",
			       pads[i].d, pads[i].x, pads[i].y);
		}

		printf("-----\n");
	}

#if 0
	for (i = 0; i < npads-1; i++) {
		if ((pads[i].x == pads[i+2].x &&
		     pads[i].y == pads[i+2].y &&
		     pads[i].d != pads[i+2].d) ||
		    (pads[i].x == pads[i+1].x &&
		     pads[i].y == pads[i+1].y &&
		     pads[i].d != pads[i+1].d))
		{
			if (0) {
				printf("D%02d %06d %06d\n",
				       pads[i].d, pads[i].x, pads[i].y);
				printf("D%02d %06d %06d\n",
				       pads[i+1].d, pads[i+1].x, pads[i+1].y);
				printf("\n");
			} else {
				lines[ pads[i].line ] = 0;

//				for (; j < npads-1; j++)
//					pads[j] = pads[j+1];
//				npads--;
//				i--;
			}
		}
	}
#else
	for (i = 0; i < npads-1; i++) {
		int bad;

		bad = 0;
		for (j = 0; j < 10; j++) {
			if (pads[i].x == pads[i+j].x &&
			    pads[i].y == pads[i+j].y &&
			    pads[i].d != pads[i+j].d)
			{
				bad++;
				fprintf(stderr, "bad @ %d,%d %d\n", pads[i].x, pads[i].y, pads[i].d);
				break;
			}
		}

		if (bad) {
			lines[ pads[i].line ] = 0;
		}
	}
#endif

	for (i = 1; i <= lineno; i++) {
		if (lines[i])
			printf("%s", lines[i]);
	}

	return 0;
}

main()
{
	parse_gerber_file(stdin);
	exit(0);
}
