FROM python:3.12-slim

WORKDIR /app

COPY requirements.txt .
RUN pip install --no-cache-dir -r requirements.txt

COPY *.py ./

# Entrypoint is set per-service in docker-compose.yml (command:)
CMD ["python", "gateway.py"]
