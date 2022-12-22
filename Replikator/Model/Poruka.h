#pragma once
//poruka koju proces salje replikatoru
typedef struct podaci {
	short procesID;
	char Podaci[100];
	struct PODACI* sledeci;
}PODACI;

void posaljiZahtevZaRegistraciju(short id);

void posaljiPodatke(short id, PODACI podaci);

void prihvatiPodatkeSaServisa();