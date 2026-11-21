ARG BUILD_FROM
FROM ${BUILD_FROM}

WORKDIR /app
COPY requirements.txt .
RUN pip3 install --no-cache-dir -r requirements.txt

COPY s3xy_ha_bridge.py run.sh ./
RUN chmod a+x /app/run.sh

CMD ["/app/run.sh"]
