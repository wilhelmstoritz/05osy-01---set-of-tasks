# Konvence pojmenování proměnných (Naming Convention)

## ✅ Implementovaná konvence

Všechny soubory projektu používají **konzistentní snake_case pojmenování** s prefixy:

### Prefixy proměnných

| Prefix | Význam | Příklady použití |
|--------|--------|------------------|
| `t_` | **Thread-local / Temporary** - vstupní parametry funkcí | `t_item`, `t_sock`, `t_format`, `t_arg` |
| `g_` | **Global** - globální proměnné | `g_buffer`, `g_mutex_sem`, `g_socket`, `g_running` |
| `l_` | **Local** - lokální proměnné uvnitř funkcí | `l_buf`, `l_len`, `l_port`, `l_name` |
| **BEZ PREFIXU** | **Iterátory ve smyčkách** - výjimka z pravidel | `i`, `j`, `k` |

### ⚠️ Výjimka: Iterátory ve smyčkách

Pro iterační proměnné ve smyčkách se **NEPOUŽÍVÁ prefix** - jednoduše `i`, `j`, `k`:

```cpp
for (int i = 0; i < 20; i++) {
    std::string l_item = "item " + std::to_string(i);
    producer(&l_item);
}

while (i < t_max_len - 1) {
    t_buf[i++] = l_c;
}
```

To je běžná konvence a iterátory jsou tak krátké a lokální, že prefix by byl zbytečný.

---

## Příklady z kódu

### 1. Globální proměnné (prefix `g_`)

```cpp
// Buffer a indexy
std::string g_buffer[N];
int g_in_index = 0;
int g_out_index = 0;

// Semafory
sem_t g_mutex_sem;
sem_t g_empty_sem;
sem_t g_full_sem;

// Socket klient
int g_socket = -1;
std::atomic<int> g_names_per_minute(DEFAULT_NAMES_PER_MIN);
std::atomic<bool> g_running(true);
```

### 2. Parametry funkcí (prefix `t_`)

```cpp
void producer(std::string* t_item);
void consumer(std::string* t_item);

void log_msg(const char* t_prefix, const char* t_format, ...);

ssize_t read_line(int t_sock, char* t_buf, size_t t_max_len);
ssize_t write_line(int t_sock, const char* t_str);

void handle_producer(int t_client_socket, const char* t_client_ip);
void* producer_thread(void* t_arg);
```

### 3. Lokální proměnné (prefix `l_`) + Iterátory (bez prefixu)

```cpp
void producer(std::string* t_item) {
    sem_wait(&g_empty_sem);
    sem_wait(&g_mutex_sem);
    insert_item(*t_item);
    sem_post(&g_mutex_sem);
    sem_post(&g_full_sem);
}

std::string consumer() {
    std::string l_item;
    sem_wait(&g_full_sem);
    sem_wait(&g_mutex_sem);
    l_item = remove_item();
    sem_post(&g_mutex_sem);
    sem_post(&g_empty_sem);
    return l_item;
}

void* producer_thread(void* t_arg) {
    for (int i = 0; i < 20; i++) {  // ← iterátor BEZ prefixu
        std::string l_item = "item " + std::to_string(i);
        producer(&l_item);
    }
    return nullptr;
}

int main(int t_argc, char** t_argv) {
    int l_port = atoi(t_argv[1]);
    int l_listen_socket = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in l_srv_addr;
    char l_buf[128];
    
    for (int i = 1; i < t_argc; i++) {  // ← iterátor BEZ prefixu
        // parse arguments
    }
}
```

---

## Výhody této konvence

### ✅ Přehlednost
- Okamžitě vidíte rozsah proměnné (scope)
- `t_` = přišlo z volající funkce
- `l_` = je to lokální, nezasahuje ven
- `g_` = globální stav, pozor na synchronizaci

### ✅ Prevence chyb
- Zamezení konfliktu názvů mezi lokálními a parametry
- Jasně viditelné globální proměnné (které potřebují synchronizaci)
- Usnadňuje refactoring

### ✅ Konzistence
- Všechny 3 hlavní soubory používají stejnou konvenci
- Snadná orientace v kódu
- Profesionální vzhled

---

## Před a po

### ❌ Před (bez konvence)
```cpp
void producer(std::string* item) {
    sem_wait(&empty_sem);
    sem_wait(&mutex_sem);
    insert_item(*item);
    sem_post(&mutex_sem);
    sem_post(&full_sem);
}

int main(int argc, char** argv) {
    int port = atoi(argv[1]);
    int socket = socket(AF_INET, SOCK_STREAM, 0);
    // ...
}
```

### ✅ Po (s konvencí)
```cpp
void producer(std::string* t_item) {
    sem_wait(&g_empty_sem);
    sem_wait(&g_mutex_sem);
    insert_item(*t_item);
    sem_post(&g_mutex_sem);
    sem_post(&g_full_sem);
}

int main(int t_argc, char** t_argv) {
    int l_port = atoi(t_argv[1]);
    int l_listen_socket = socket(AF_INET, SOCK_STREAM, 0);
    
    for (int i = 1; i < t_argc; i++) {  // iterátor bez prefixu
        // ...
    }
}
```

---

## Další konvence v projektu

### Snake_case
- ✅ Všechny proměnné: `g_buffer`, `l_client_socket`
- ✅ Všechny funkce: `read_line()`, `insert_item()`
- ✅ Struktury: `client_data`

### UPPER_CASE
- ✅ Makra: `#define N 10`
- ✅ Konstanty: `#define STR_QUIT "quit"`
- ✅ Log levely: `LOG_INFO`, `LOG_ERROR`

---

## Soubory s implementovanou konvencí

✅ `interprocess-communication.cpp` - kompletně přepracováno  
✅ `socket_srv.cpp` - kompletně přepracováno  
✅ `socket_cl.cpp` - kompletně přepracováno  
✅ `verify.sh` - aktualizován pro nové názvy semaforů  

---

## Testování

Program byl otestován po přepracování a všechny testy prošly:

```bash
$ ./verify.sh
✅ TEST 1: Kompilace programů - ÚSPĚŠNÁ
✅ TEST 2: Kontrola vytvořených programů - OK
✅ TEST 3: Kontrola souboru jmena.txt - OK
✅ TEST 4: Základní producer-consumer - FUNGUJE
✅ TEST 5: Kontrola 3 semaforů - OK (g_mutex_sem, g_empty_sem, g_full_sem)
✅ TEST 6: Kontrola signatur funkcí - OK
✅ TEST 7-10: Další testy - OK
```

---

## Závěr

Kód nyní používá **konzistentní a profesionální konvenci pojmenování** s prefixy `t_`, `g_`, `l_`, která:
- Zlepšuje čitelnost
- Snižuje riziko chyb
- Usnadňuje údržbu
- Odpovídá moderním standardům C/C++ projektů

Veškerá funkcionalita zůstává zachována - **100% kompatibilita s původním kódem**.
