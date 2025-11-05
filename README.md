# Custom Shell (C++ Linux Shell)

A custom Unix-like command shell written in **C++**, featuring:

* Command parsing and execution
* Pipelines (`|`)
* Input/Output redirection (`<`, `>`, `>>`)
* Background execution (`&`)
* Built-in commands: `cd`, `exit`, `jobs`, `fg`, `bg`
* Basic job control with process groups and signals
* Colorized, informative prompt

This project is designed for systems programming and DevOps learning and is fully containerized to run anywhere.

---

## 🚀 Run with Docker (Recommended)

No setup needed — just run:

```
docker run -it shuvra458/custom-shell:latest
```

---

## 🔧 Build & Run Locally

### Requirements

* Linux environment
* g++ (C++17)
* make
* readline development library

### Install Dependencies (Ubuntu / Debian)

```
sudo apt update
sudo apt install -y g++ make libreadline-dev
```

### Compile

```
make
```

### Run Shell

```
./bin/myshell
```

---

## 📝 Usage Examples

```
# Simple command
ls -l

# Pipeline
ls | grep cpp

# Output redirection
ls > out.txt

# Background job
sleep 10 &

# Job control
jobs
fg 1
```

---

## 🛠 Built-in Commands

| Command    | Description                  |
| ---------- | ---------------------------- |
| `cd <dir>` | Change directory             |
| `exit`     | Quit shell                   |
| `jobs`     | List background/stopped jobs |
| `fg <id>`  | Bring job to foreground      |
| `bg <id>`  | Resume job in background     |

---

## 🐳 CI/CD with Jenkins + DockerHub

A Jenkins Pipeline is included to:

* Build the shell executable
* Build a Docker image
* Push the image to DockerHub automatically

See `Jenkinsfile` for details.

---

## 📂 Project Structure

```
custom-shell/
├── src/
│   ├── main.cpp
│   ├── shell.cpp
│   ├── parser.cpp
│   |── jobs.cpp
│   ├── shell.h
│   ├── parser.h
│   └── jobs.h
├── bin/
├── Makefile
├── Dockerfile
└── Jenkinsfile
```

---

## 🤝 Contributing

Pull requests are welcome — improve features, add commands, enhance job control.

---

## 📜 License

MIT License

---

### ⭐ If you like this project, star the repository and share it!
