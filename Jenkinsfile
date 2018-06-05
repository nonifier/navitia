pipeline {
  agent any
  stages {
    stage('Build') {
      agent any
      steps {
        git(url: 'git@github.com:nonifier/navitia_docker_images.git', branch: 'ubuntu_18')
        sh 'pwd'
        sh 'ls -l'
        sh 'find ../ -iname dockerfile'
      }
    }
  }
}