pipeline {
    agent any

    environment {
        DOCKERHUB_REPO = "shuvra458/custom-shell"
    }

    stages {
        stage('Clone') {
            steps {
                checkout scm
            }
        }

        stage('Build Docker Image') {
            steps {
                script {
                    sh 'docker build -t custom-shell .'
                }
            }
        }

        stage('Tag Image') {
            steps {
                script {
                    sh 'docker tag custom-shell $DOCKERHUB_REPO:latest'
                }
            }
        }

        stage('Push to DockerHub') {
            steps {
                withCredentials([usernamePassword(credentialsId: 'docker-hub-cred', usernameVariable: 'DOCKER_USER', passwordVariable: 'DOCKER_PASS')]) {
                    sh """
                    echo "$DOCKER_PASS" | docker login -u "$DOCKER_USER" --password-stdin
                    docker push $DOCKERHUB_REPO:latest
                    """
                }
            }
        }
    }
}