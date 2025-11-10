# Producer-Consumer s POSIX semafory a Socket Server/Client

## Popis řešení

Implementace klasického producer-consumer problému pomocí **3 POSIX semaforů** podle knihy Modern Operating Systems (obrázek 2-28, strana 131), rozšířená o **soketový server s vlákny** a **inteligentní klienty**.

## Část 1: Základní Producer-Consumer

### Semafory (přesně 3 jako v knize)
- **`mutex_sem`** - ochrana kritické sekce (inicializováno na 1)
- **`empty_sem`** - počet volných slotů v bufferu (inicializováno na N)
- **`full_sem`** - počet obsazených slotů v bufferu (inicializováno na 0)

### Buffer
- Implementováno jako **kruhový buffer** (circular buffer) pomocí pole `std::string buffer[N]`
- Indexy `in_index` a `out_index` pro správu kruhového bufferu
- Velikost bufferu: N = 10 položek

### Funkce producer a consumer
Podle zadání **bez while smyčky**, s parametrem:

```cpp
void producer(std::string* item);  // vloží jednu položku do bufferu
void consumer(std::string* item);  // vyjme jednu položku z bufferu
```

Typ předávaných dat: **std::string** (řetězec)

### Algoritmus (podle obrázku 2-28)

**Producer:**
1. `sem_wait(&empty_sem)` - čekáme na volné místo
2. `sem_wait(&mutex_sem)` - vstup do kritické sekce
3. Vložení položky do bufferu
4. `sem_post(&mutex_sem)` - opuštění kritické sekce
5. `sem_post(&full_sem)` - signalizace nové položky

**Consumer:**
1. `sem_wait(&full_sem)` - čekáme na položku v bufferu
2. `sem_wait(&mutex_sem)` - vstup do kritické sekce
3. Vyjmutí položky z bufferu
4. `sem_post(&mutex_sem)` - opuštění kritické sekce
5. `sem_post(&empty_sem)` - signalizace uvolněného místa

## Část 2: Socket Server s vlákny

Server implementuje producer-consumer přes síť:

### Vlastnosti serveru:
- **Jeden socket server** na zadaném portu
- **Vlákno pro každého klienta** (ne procesy!)
- **Sdílený buffer** mezi všemi klienty (chráněný semafory)
- Server se ptá každého klienta: `Task?\n`
- Klient odpovídá: `producer\n` nebo `consumer\n`

### Protokol:
**Producer klient → Server:**
- Klient posílá: `jmeno\n`
- Server odpovídá: `OK\n`
- Server vloží jméno do bufferu

**Server → Consumer klient:**
- Server posílá: `jmeno\n`
- Klient odpovídá: `OK\n`
- Server vyjme jméno z bufferu

## Část 3: Inteligentní Socket Client

Klient s vlákny podle role:

### Producer role:
- **Vlákno pro odesílání**: Načte soubor `jmena.txt` a odesílá jména na server
- **Vlákno pro stdin**: Umožňuje měnit rychlost odesílání (jmen za minutu)
- **Výchozí rychlost**: 60 jmen/minutu
- **Dynamická změna**: Zadáním čísla do stdin (např. `120` = 120 jmen/min)

### Consumer role:
- **Vlákno pro příjem**: Přijímá jména ze serveru a zobrazuje je
- Automaticky odpovídá `OK\n` na každé jméno
- Bez omezení rychlosti (přijímá jak rychle server posílá)

## Kompilace

```bash
make              # zkompiluje všechny programy
make clean        # vyčistí
```

Nebo jednotlivě:
```bash
make interprocess-communication  # základní verze
make socket_srv                  # server
make socket_cl                   # klient
```

## Spuštění

### 1. Základní producer-consumer (bez sítě)
```bash
make run
# nebo
./interprocess-communication
```

### 2. Server
```bash
make run-server
# nebo
./socket_srv 12345
```

### 3. Klient (producer)
V jiném terminálu:
```bash
./socket_cl localhost 12345
# Zadejte: producer
# Pak můžete měnit rychlost číslem (např. 120)
```

### 4. Klient (consumer)
V dalším terminálu:
```bash
./socket_cl localhost 12345
# Zadejte: consumer
```

## Testovací skripty

```bash
# Automatický test s více klienty
./test-complete.sh [port]

# Interaktivní producer
./test-producer-interactive.sh [host] [port]

# Interaktivní consumer
./test-consumer-interactive.sh [host] [port]
```

## Soubor jmena.txt

Nachází se v: `../kelvin/jmena.txt`

Obsahuje 201 českých křestních jmen (Adam, Adela, Adrian, ...).
Producer klient je načítá a odesílá cyklicky.

## Klíčové vlastnosti

✅ **Přesně 3 POSIX semafory** (sem_t) - žádné další mutexy, zámky, condition variables  
✅ **Funkce bez while smyček** - producer/consumer přijímají item jako parametr  
✅ **Typ dat: std::string** (řetězec)  
✅ **Kruhový buffer** - efektivní využití paměti, O(1) operace  
✅ **POSIX vlákna** (pthread) - ne std::thread  
✅ **Správná synchronizace** - bez race conditions  
✅ **Soketový server s vlákny** - každý klient = nové vlákno  
✅ **Sdílený buffer** - všichni klienti přistupují ke stejnému bufferu  
✅ **Dynamická rychlost** - producer může měnit rychlost za běhu  
✅ **Načítání ze souboru** - jména z jmena.txt  

## Příklad výstupu

### Server:
```
Server listening on port 12345
Buffer size: 10
Client connected from 127.0.0.1:54321
Client chose role: producer
Client connected from 127.0.0.1:54322
Client chose role: consumer
Produced: Adam
Consumed: Adam
Produced: Adela
Consumed: Adela
...
```

### Producer klient:
```
Connected to server.
Server asks: Task?
Enter 'producer' or 'consumer': producer
Role selected: producer
Loaded 201 names from file.
Enter number to change names per minute (current: 60):
Sent: Adam
Sent: Adela
120         <- uživatel změní rychlost
Names per minute changed to: 120
Sent: Adrian
...
```

### Consumer klient:
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
```

## Proč kruhový buffer?

**Výhody:**
- ✅ Konstantní časová složitost O(1) pro insert i remove
- ✅ Žádné dynamické alokace
- ✅ Žádné posouvání dat (na rozdíl od vectoru s erase)
- ✅ Opakované využití stejné paměti
- ✅ Standardní implementace pro producer-consumer
- ✅ Odpovídá příkladu z knihy Modern Operating Systems

