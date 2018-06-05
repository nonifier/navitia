pipeline {
  agent any
  stages {
    stage('Build') {
      agent any
      steps {
        echo 'Let\'s build navitia'
        sh 'echo "Shell Script !"'
        git(url: 'git@github.com:CanalTP/navitia_docker_images.git', branch: 'ubuntu_18')
      }
    }
  }
}