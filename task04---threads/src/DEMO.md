# Demonstrace Producer-Consumer Socket Server

## Scénář popsaný v zadání

### Krok 1: Producer naplní buffer a zastaví se

**Terminál 1 - Server:**
```bash
./socket_srv 12345
```

Výstup:
```
Producer-Consumer Socket Server
Buffer size: 10
Listening on port: 12345
```

**Terminál 2 - Producer klient:**
```bash
./socket_cl localhost 12345
```

Zadejte: `producer`

Výstup:
```
Connected to server.
Server asks: Task?
Enter 'producer' or 'consumer': producer
Role selected: producer
Loaded 201 names from file.
Producer thread started.
Sending ~60 names per minute.
Enter number to change rate:

Sent: Adam (OK)
Sent: Adela (OK)
Sent: Adrian (OK)
...
Sent: Alice (OK)
Sent: Alois (OK)
[BLOCKED - buffer full, waiting for consumer...]
```

**Co se stalo:**
- Producer odeslal 10 jmen (velikost bufferu)
- Buffer je plný
- `sem_wait(&empty_sem)` blokuje - čeká na volné místo
- Producer thread je pozastavený

### Krok 2: Odpojíme producera

**Terminál 2:**
```
Ctrl+C
```

Producer se odpojí, buffer na serveru zůstává plný s 10 jmény.

### Krok 3: Consumer vyprázdní buffer a zastaví se

**Terminál 3 - Consumer klient:**
```bash
./socket_cl localhost 12345
```

Zadejte: `consumer`

Výstup:
```
Connected to server.
Server asks: Task?
Enter 'producer' or 'consumer': consumer
Role selected: consumer
Consumer thread started.

Received: Adam
Received: Adela
Received: Adrian
...
Received: Alice
Received: Alois
[BLOCKED - buffer empty, waiting for producer...]
```

**Co se stalo:**
- Consumer přijal všech 10 jmen z bufferu
- Buffer je prázdný
- `sem_wait(&full_sem)` blokuje - čeká na data
- Consumer thread je pozastavený

### Krok 4: Odpojíme consumera

**Terminál 3:**
```
Ctrl+C
```

Consumer se odpojí.

---

## Scénář: Více producentů a konzumentů paralelně

**Terminál 1 - Server:**
```bash
./socket_srv 12345
```

**Terminál 2 - Producer 1:**
```bash
./socket_cl localhost 12345
```
Zadejte: `producer`

**Terminál 3 - Producer 2:**
```bash
./socket_cl localhost 12345
```
Zadejte: `producer`

**Terminál 4 - Consumer 1:**
```bash
./socket_cl localhost 12345
```
Zadejte: `consumer`

**Terminál 5 - Consumer 2:**
```bash
./socket_cl localhost 12345
```
Zadejte: `consumer`

### Očekávané chování:

**Server log:**
```
Client [thread 1] connected - role: producer
Client [thread 2] connected - role: producer
Client [thread 3] connected - role: consumer
Client [thread 4] connected - role: consumer

[Thread 1] Produced: Adam
[Thread 3] Consumed: Adam
[Thread 2] Produced: Adela
[Thread 4] Consumed: Adela
[Thread 1] Produced: Adrian
[Thread 3] Consumed: Adrian
...

Buffer status: 3/10 items
```

**Producer 1 mění rychlost:**
V terminálu 2 zadejte: `120`
```
Names per minute changed to: 120
```
Teď odesílá 2 jména za sekundu!

**Synchronizace:**
- ✅ Oba producenti mohou přidávat současně (střídají se v mutex)
- ✅ Oba consumenti mohou odebírat současně (střídají se v mutex)
- ✅ Pokud buffer je plný, producenti čekají
- ✅ Pokud buffer je prázdný, consumenti čekají
- ✅ Žádná jména se neztratí ani nezdvojí

---

## Debug výpisy serveru

Server může zobrazovat:

```
=== Buffer Status ===
Current: 7/10 items
Producers waiting: 0
Consumers waiting: 1
====================

[Thread 2] Producer blocked - buffer full
[Thread 3] Consumer blocked - buffer empty
[Thread 1] Produced: Karel → buffer: 8/10
[Thread 4] Consumed: Martin → buffer: 7/10
```

---

## Testování s netcat (ruční test)

```bash
# Terminál 1: Server
./socket_srv 12345

# Terminál 2: Manuální producer
nc localhost 12345

# Server pošle: Task?
# Odpovíte: producer
# Server: OK
# Pak posílejte jména:
Jan
Karel
Petr
^C

# Terminál 3: Manuální consumer
nc localhost 12345

# Server pošle: Task?
# Odpovíte: consumer
# Server pošle jména, vy odpovídejte OK:
# Server: Jan
OK
# Server: Karel
OK
# Server: Petr
OK
```

---

## Ověření správnosti

### 1. Race conditions
**Test:** Spusťte 5 producentů a 5 konsumentů současně
**Očekávané:** Žádná data se neztratí, nezdvojí, buffer zůstane konzistentní

### 2. Deadlock
**Test:** Naplňte buffer a odpojte všechny konzumenty
**Očekávané:** Producenti se korektně zablokují, ne deadlock

### 3. Buffer overflow
**Test:** Rychle posílejte data z více producentů
**Očekávané:** Semafory zajistí, že buffer nikdy nepřeteče (max 10 položek)

### 4. Buffer underflow
**Test:** Rychle konzumujte bez producentů
**Očekávané:** Consumenti se zablokují, nepokusí se číst z prázdného bufferu

---

## Klíčové vlastnosti implementace

✅ **Blokující chování:** Producer/consumer se automaticky zastaví při plném/prázdném bufferu
✅ **Thread-safe:** Správná synchronizace pomocí 3 semaforů
✅ **Škálovatelné:** Neomezený počet producentů i konzumentů
✅ **Fair:** FIFO pořadí díky kruhovému bufferu
✅ **Efektivní:** O(1) operace, žádné aktivní čekání
