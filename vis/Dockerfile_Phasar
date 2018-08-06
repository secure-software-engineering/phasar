FROM phasar


RUN apt-get update && apt-get install -y curl \
    && curl -sL https://deb.nodesource.com/setup_8.x | bash - \
    && apt-get install -y git nodejs \
    && npm install -g typescript \
    && apt-get install apt-transport-https \
    && curl -sS https://dl.yarnpkg.com/debian/pubkey.gpg | apt-key add - \
    && echo "deb https://dl.yarnpkg.com/debian/ stable main" | tee /etc/apt/sources.list.d/yarn.list \
    && apt-get update && apt-get install yarn \
    && groupadd -r app && useradd -r -m -g app app

ADD . /opt/exploded-supergraph-visualisation-framework

RUN cd /opt/exploded-supergraph-visualisation-framework \
    && yarn install \
    && tsc \
    && chown -R app:app /opt/exploded-supergraph-visualisation-framework \
    && apt-get remove --purge -y git \
    && apt-get autoremove -y --purge \
    && apt-get clean -y \
    && rm -rf /var/lib/apt/lists/* 

RUN echo '#!/bin/bash' > /bin/entrypoint.sh \
    && echo "sed -i \"s/mongodb:\/\/localhost/mongodb:\/\/\${MONGO_HOST}/\" /opt/exploded-supergraph-visualisation-framework/src/server/config/config.ts" >> /bin/entrypoint.sh \
    && echo "sed -i \"s/sse_dfa_llvm/\${FRAMEWORK_CWD}/g\" /opt/exploded-supergraph-visualisation-framework/src/server/config/config.ts" >> /bin/entrypoint.sh \
    && echo "\$@" >> /bin/entrypoint.sh \
    && chmod +x /bin/entrypoint.sh

USER app

EXPOSE 3000
ENV MONGO_HOST localhost
ENV FRAMEWORK_CWD \\/opt\\/framework

WORKDIR /opt/exploded-supergraph-visualisation-framework
RUN mkdir -p server/data/uploads

ENTRYPOINT ["/bin/entrypoint.sh"]
CMD ["yarn", "start-prod"]
