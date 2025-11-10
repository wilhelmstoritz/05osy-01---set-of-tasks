# Producer-Consumer s POSIX semafory

## Popis řešení

Implementace klasického producer-consumer problému pomocí **3 POSIX semaforů** podle knihy Modern Operating Systems (obrázek 2-28, strana 131).

## Implementace

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

## Kompilace a spuštění

```bash
make        # kompilace
make run    # kompilace a spuštění
make clean  # vyčištění
```

Nebo přímo:
```bash
g++ -Wall -Wextra -std=c++11 -pthread -o interprocess-communication interprocess-communication.cpp
./interprocess-communication
```

## Klíčové vlastnosti

✅ **Přesně 3 POSIX semafory** (sem_t) - žádné další mutexy, zámky, condition variables  
✅ **Funkce bez while smyček** - producer/consumer přijímají item jako parametr  
✅ **Typ dat: std::string** (řetězec)  
✅ **Kruhový buffer** - efektivní využití paměti  
✅ **POSIX vlákna** (pthread) - ne std::thread  
✅ **Správná synchronizace** - bez race conditions  

## Výstup programu

Program vytvoří 2 vlákna (producer a consumer), které pracují s sdíleným bufferem:

```
Starting producer-consumer with buffer size: 10
Produced: Item_0
Consumed: Item_0
Produced: Item_1
Consumed: Item_1
...
Producer-consumer finished.
```

Producer produkuje položky rychleji (100ms), consumer je konzumuje pomaleji (150ms), což demonstruje efektivitu bufferování.
