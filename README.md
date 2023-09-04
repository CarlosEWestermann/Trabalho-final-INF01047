# Relatório do Trabalho Final de Fundamentos de Computação Gráfica
O objetivo do jogo é coletar todas as moedas do mapa sem que a nave colida com algum asteroide. As moedas devem ser coletadas em ordem, de forma que só é possível coletar uma moeda após ter coletado a anterior. Pensando nisso, a câmera look-at aponta sempre para a próxima moeda que o jogador deve coletar. Ao coletar todas as moedas, o jogador vence. Caso colida com um asteroide, o jogador perde. Além disso, é possível atirar mísseis contra os asteroides para destruí-los. 

## Manual:
- **W**: Movimenta a nave para frente.
- **S**: Movimenta a nave para trás.
- **A**: Nave realiza um rolamento a esquerda.
- **D**: Nave realiza um rolamento a direita.
- **F**: Troca de câmera livre para look-at.
- **Clique botão esquerdo do mouse + movimentar o mouse**: Controla a direção da nave.
- **Space**: Atira um míssil.
- **Enter**: Começa uma partida, quando está na tela inicial.
- **ESC**: Fecha o jogo.


## Como compilar:
### Em ambiente linux:
Para compilar e executar este projeto no Linux, primeiro você precisa instalar as bibliotecas necessárias. Isso é normalmente suficiente em uma instalação de Linux Ubuntu:
```
sudo apt-get install build-essential make libx11-dev libxrandr-dev 
libxinerama-dev libxcursor-dev libxcb1-dev libxext-dev 
libxrender-dev libxfixes-dev libxau-dev libxdmcp-dev
```	      
Com as bibliotecas instaladas, em um terminal, navegue até o diretório `game` do projeto. Para compilar utilize o comando `make`. Para executar, utilize `make run`. Tambem é possivel acessar o binario compilado em `game/bin/linux/`.


## Contribuições
### As contribuições do Pedro envolvem: 
- Modelagem da nave, da esfera correspondente ao “universo”, dos mísseis e das moedas.
- Animação dos meteoros utilizando curvas de bézier cúbicas.
- Animação dos asteroides se movendo em conjunto. 
- Câmera look-at.
- Posicionamento dos objetos para criação do mapa. 
- Implementação da lógica 

### As contribuições do Carlos envolvem:
- Testes de colisão esfera-esfera, esfera-cubo, esfera-plano(círculo).
- Câmera livre.
- Movimentação da nave, incluindo Barrel Roll.
- Modelos de iluminação e de interpolação.
- Modelagem dos asteroides.

### Contribuições conjuntas:
- Texturas em todos objetos.
- Animação dos mísseis.

Utilizamos o Chatgpt em alguns momentos pontuais durante o desenvolvimento do código para tentar auxiliar a entender o porquê algo não funcionava, como por exemplo durante o desenvolvimento da câmera livre, ou para pesquisar descrições matemáticas, como na implementação do teste de colisão esfera-plano em que a sugestão de como calcular a distância de um ponto para o plano fazia sentido. De forma geral, para gerar código a ferramenta não foi muito útil, visto que não aproveitamos nada do que ela sugeriu, mas para a análise de erros foi.

## Processo de desenvolvimento:
Iniciamos o processo de desenvolvimento com base no Laboratório 5, que já apresentava uma implementação para o mapeamento de texturas em uma esfera. Nossa primeira tarefa consistiu em reaproveitar o código para construir o plano de fundo do jogo, utilizando uma textura de estrelas.
Os próximos passos envolveram a modelagem dos objetos, incluindo a nave, os asteroides, as moedas e os mísseis. Aqui, a parte complicada foi encontrar os modelos gratuitos na internet que satisfaziam nossos interesses.
Uma vez que já tínhamos isso pronto, avançamos para a etapa de animação. Isso engloba a movimentação da nave, a animação do tiro, a chuva de meteoros e o deslocamento em conjunto de um grupo de asteroides. Nesse estágio, também focamos em desenvolver a detecção de colisões e os modelos de iluminação, pensando em já cumprir todas as especificações propostas pelo professor.
Na etapa final do desenvolvimento, dedicamos nossos esforços à implementação da essência do jogo, envolvendo os sistemas de pontuação, bem como a lógica que determina as condições de vitória e derrota. Esta fase também incluiu a concepção do mapa, que abrange o posicionamento dos objetos, pensando em criar um ambiente minimamente desafiador. 
Para desenvolver este projeto, nós dividimos as tarefas de forma balanceada para evitar sobrecargas individuais. Mantivemos uma comunicação constante, sempre abertos a ajudar um ao outro. Após a conclusão de uma tarefa específica, garantíamos a integração do código, realizando pushs no repositório remoto que permitiam ao colega atualizar seu repositório local e testar as modificações implementadas.

## Uso dos conceitos de Computação Gráfica:
Para implementar a chuva de meteoros, utilizamos curvas de bézier cúbicas definidas a partir de pontos de controle com um certo nível de aleatoriedade, para que os meteoros tenham cursos diferentes.
Para implementar o deslocamento em conjunto de um grupo de asteroides, utilizamos instâncias definidas a partir de uma mesma matriz de modelagem base, que translada para a direita com o passar do tempo.
Para implementar os objetos que se movem durante o jogo, como por exemplo o player e os tiros, calculamos as animações baseadas em um delta de tempo, de forma que ocorrem sempre na mesma velocidade independente da velocidade da CPU.
Para implementar a visão do player padrão, utilizamos a câmera livre, e para a visão da pista, utilizamos a câmera look-at.
Para implementar o foguete, utilizamos a interpolação de Gouraud e para os demais objetos utilizamos de Phong.
Para os asteroides utilizamos o modelo de Blinn-Phong e para os outros objetos utilizamos Lambert.
Para fazer as colisões entre a nave e os asteroides utilizamos colisão esfera-esfera, para os mísseis e o asteroide utilizamos esfera-cubo e para a nave e as moedas utilizamos esfera-plano.
Os barrel rolls controlados pelo usuário aplicam transformações geométricas sobre a nave espacial.
A malha poligonal da nave é composta por 55.120 triângulos, que a faz mais complexa do que a da cow.obj.
Trivialmente, todos objetos têm texturas mapeadas.

### Start screen:
![Alt](/images/start.png "Start screnn")

### Lookat camera view:
![Alt](/images/lookat.png "lookat view")

### Free camera view:
![Alt](/images/map.png "initial ship view")

### Ship Barrel roll:
![Alt](/images/flip.png "Ship Barrel roll")
