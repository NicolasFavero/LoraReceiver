# Faz o upload normal (botao "Upload"/pio run -t upload) subir o
# filesystem (data/) automaticamente antes -- assim nao tem como
# esquecer o `--target uploadfs` depois de mexer na pagina web (ou
# na primeira gravacao de uma placa nova).
#
# Custo: todo upload demora alguns segundos a mais (grava a imagem
# do filesystem de novo, mesmo se data/ nao mudou). Isso NAO desgasta
# a flash de forma relevante -- a flash SPI do ESP32 aguenta algo
# como 100.000 ciclos de escrita por setor, e o LittleFS já faz wear
# leveling (distribui as escritas). Uma pagina de ~5-10KB, mesmo
# regravada em todo upload durante meses de desenvolvimento, fica
# muito abaixo desse limite.

Import("env")


def before_upload(source, target, env):
    print("[auto_uploadfs] Subindo o filesystem (data/) antes do firmware...")
    env.Execute(
        "\"$PYTHONEXE\" -m platformio run -t uploadfs -e " + env["PIOENV"]
    )


env.AddPreAction("upload", before_upload)
