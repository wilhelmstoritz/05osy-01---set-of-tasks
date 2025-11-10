# Producer-Consumer Socket Server - Testovací scénáře

## Příprava

```bash
# Kompilace
make

# Spuštění serveru (v samostatném terminálu)
./socket_srv 12345
```

## Scénář 1: Producer naplní buffer

V novém terminálu:
```bash
cd task04---threads/src
echo "producer" | ./socket_cl localhost 12345
```

**Očekávané chování:**
- Klient se připojí jako producer
- Začne odesílat jména ze souboru `jmena.txt`
- Po naplnění bufferu (10 položek) se **zastaví** (blokuje na `sem_wait(&empty_sem)`)
- Server zobrazí: "Buffer full, producer waiting..."

## Scénář 2: Consumer vyprázdní buffer

V dalším terminálu:
```bash
cd task04---threads/src
echo "consumer" | ./socket_cl localhost 12345
```

**Očekávané chování:**
- Klient se připojí jako consumer
- Začne přijímat jména ze serveru
- Přečte všech 10 jmen z bufferu
- Po vyprázdnění bufferu se **zastaví** (blokuje na `sem_wait(&full_sem)`)
- Server zobrazí: "Buffer empty, consumer waiting..."

## Scénář 3: Více producentů a konzumentů paralelně

V různých terminálech spusťte současně:

**Terminál 1-2: Dva producenti**
```bash
echo "producer" | ./socket_cl localhost 12345
```

**Terminál 3-4: Dva consumenti**
```bash
echo "consumer" | ./socket_cl localhost 12345
```

**Očekávané chování:**
- Oba producenti odesílají jména současně
- Oba consumenti přijímají jména současně
- Semafory zajistí správnou synchronizaci
- Žádná jména se neztratí ani nezdvojí
- Buffer osciluje mezi prázdným a plným stavem

## Nastavení rychlosti producera

Během běhu producenta můžete v jeho terminálu zadat číslo pro změnu rychlosti:

```bash
# Spustit producera
echo "producer" | ./socket_cl localhost 12345

# V průběhu zadat (do stdin):
120     # 120 jmen za minutu (2x rychleji)
30      # 30 jmen za minutu (2x pomaleji)
```

## Manuální test s netcat

Můžete také testovat ručně s netcat:

```bash
nc localhost 12345
# Server pošle: Task?
producer
# Pak odesílejte jména:
Jan
Karel
Petr
^C
```

## Kontrola logů

Server vypisuje průběžné informace:
- Počet připojených klientů
- Role každého klienta (producer/consumer)
- Aktuální stav bufferu
- Blocking operace (producer waiting, consumer waiting)

## Ukončení

- Server: stiskněte Ctrl+C nebo zadejte 'quit'
- Klient: stiskněte Ctrl+C
- Nebo: `killall socket_srv socket_cl`
