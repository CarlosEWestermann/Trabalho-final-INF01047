# Trabalho-final-INF01047

As contribuições do Pedro envolvem: 
1. Modelagem da nave, da esfera correspondente ao “universo”, dos mísseis e das moedas.
2. Animação dos meteoros utilizando curvas de bézier cúbicas.
3. Animação dos asteroides se movendo em conjunto. 
4. Câmera look-at.
5. Posicionamento dos objetos para criação do mapa. 
6. Implementação da lógica 

As contribuições do Carlos envolvem:
1.Testes de colisão esfera-esfera, esfera-cubo, esfera-plano(círculo).
2. Câmera livre.
3. Movimentação da nave, incluindo Barrel Roll.
4. Modelos de iluminação e de interpolação.
5. Modelagem dos asteroides.

Contribuições conjuntas:
1. Texturas em todos objetos.
2. Animação dos mísseis.

Utilizamos o chatgpt em alguns momentos pontuais durante o desenvolvimento do código para tentar auxiliar a entender o porquê algo não funcionava, como por exemplo durante o desenvolvimento da câmera livre. De forma geral, para gerar código a ferramenta não foi muito útil, visto que não aproveitamos quase nada do que ela sugeriu. Por outro lado, ela foi capaz de auxiliar a entender possíveis motivos para algo não funcionar, assim como descrições matemáticas, como na colisão esfera-plano em que a sugestão de como calcular a distância de um ponto para o plano fazia sentido. De forma geral, as respostas não são consistentes e é necessário analisar as respostas que ele entrega. Para escrever código a ferramenta não foi útil, mas para a análise de erros foi.










Processo de desenvolvimento

Iniciamos o processo de desenvolvimento com base no Laboratório 5, que já apresentava uma implementação para o mapeamento de texturas em uma esfera. Nossa primeira tarefa consistiu em reaproveitar o código para construir o plano de fundo do jogo, utilizando uma textura de estrelas.
Os próximos passos envolveram a modelagem dos objetos, incluindo a nave, os asteroides, as moedas e os mísseis. Aqui, a parte complicada foi encontrar os modelos gratuitos na internet que satisfaziam nossos interesses.
Uma vez que já tínhamos isso pronto, avançamos para a etapa de animação. Isso engloba a movimentação da nave, a animação do tiro, a chuva de meteoros e o deslocamento em conjunto de um grupo de asteroides. Nesse estágio, também focamos em desenvolver a detecção de colisões e os modelos de iluminação, pensando em já cumprir todas as especificações propostas pelo professor.
Na etapa final do desenvolvimento, dedicamos nossos esforços à implementação da essência do jogo, envolvendo os sistemas de pontuação, bem como a lógica que determina as condições de vitória e derrota. Esta fase também incluiu a concepção do mapa, que abrange o posicionamento dos objetos, pensando em criar um ambiente minimamente desafiador. 
Para desenvolver este projeto, nós dividimos as tarefas de forma balanceada para evitar sobrecargas individuais. Mantivemos uma comunicação constante, sempre abertos a ajudar um ao outro. Após a conclusão de uma tarefa específica, garantíamos a integração do código, realizando pushs no repositório remoto que permitiam ao colega atualizar seu repositório local e testar as modificações implementadas.







Uso dos conceitos de Computação Gráfica


Para implementar a chuva de meteoros, utilizamos curvas de bézier cúbicas definidas a partir de pontos de controle com um certo nível de aleatoriedade, para que os meteoros tenham cursos diferentes.
Para implementar o deslocamento em conjunto de um grupo de asteroides, utilizamos instâncias definidas a partir de um mesmo  , que translada
Para implementar os objetos que se movem durante o jogo, como por exemplo o player e os tiros, calculamos as animações baseadas em um delta de tempo, de forma que ocorrem sempre na mesma velocidade independente da velocidade da CPU.
Para implementar a visão do player padrão, utilizamos a câmera livre, e para a visão da pista, utilizamos a câmera look-at.
Para implementar o foguete, utilizamos a interpolação de Gouraud e para os demais objetos utilizamos de Phong.
Para os asteroides utilizamos o modelo de Blinn-Phong e para os outros objetos utilizamos Lambert.
Para fazer as colisões entre a nave e os asteroides utilizamos colisão esfera-esfera, para os mísseis e o asteroide utilizamos esfera-cubo e para a nave e as moedas utilizamos esfera-plano.
Os barrel rolls controlados pelo usuário aplicam transformações geométricas sobre a nave espacial.
A malha poligonal da nave é composta por 55.120 triângulos, que a faz mais complexa do que a da cow.obj.
Trivialmente, todos objetos têm texturas mapeadas.

Manual:


Controles:\
W: Movimenta a câmera do jogador para frente no modo câmera livre.\
S: Movimenta a câmera do jogador para trás no modo câmera livre.\
A: Faz um “Barrel roll” para esquerda.\
D Faz um “Barrel roll” para direita.\
F: troca de câmera livre para look-at.\
Espaço: atira um míssil.\
Click + movimentar o mouse: movimenta a câmera para olhar na direção do mouse na câmera livre.\
ESC: fecha o jogo.\

Objetivo do jogo:\
	O objetivo do jogo é coletar todas as moedas do mapa sem que a nave colida com algum asteroide. As moedas devem ser coletadas em ordem, de forma que só é possível coletar a segunda após coletar a primeira. A cada coleta de moeda, a câmera look-at é posicionada na posição da moeda para que o jogador possa visualizar o que resta do mapa. Ao coletar todas as moedas, o jogador vence. Caso colida com um asteroide ele perde o jogo. Além disso, o jogador pode atirar mísseis contra os asteroides para destruí-los. 




