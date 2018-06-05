pipeline {
  agent any
  stages {
    stage('Build') {
      agent any
      steps {
        git(url: 'git@github.com:nonifier/navitia_docker_images.git', branch: 'ubuntu_18', credentialsId: ' 0620e9c03baef1a2d080fcf50da3d8853ac20673')
        sh 'pwd'
        sh 'ls -l'
        sh 'find ../ -iname dockerfile'
      }
    }
  }
}