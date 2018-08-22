FROM library/node:slim

EXPOSE 3000

ADD . /opt/exploded-supergraph-visualisation-framework

RUN apt-get update && apt-get install -y git \
    && npm install -g typescript \
    && apt-get install apt-transport-https \
    && curl -sS https://dl.yarnpkg.com/debian/pubkey.gpg | apt-key add - \
    && echo "deb https://dl.yarnpkg.com/debian/ stable main" | tee /etc/apt/sources.list.d/yarn.list \
    && apt-get update && apt-get install yarn \
    && groupadd -r app && useradd -r -m -g app app \
    && cd /opt/exploded-supergraph-visualisation-framework \
    && yarn install \
    && tsc \
    && chown -R app:app /opt/exploded-supergraph-visualisation-framework \
    && apt-get remove --purge -y git \
    && apt-get autoremove -y --purge \
    && apt-get clean -y \
    && rm -rf /var/lib/apt/lists/* 

USER app

WORKDIR /opt/exploded-supergraph-visualisation-framework

CMD ["yarn", "start"]
