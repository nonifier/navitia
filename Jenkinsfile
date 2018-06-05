pipeline {
  agent any
  stages {
    stage('Build') {
      agent {
        dockerfile {
          filename 'https://raw.githubusercontent.com/CanalTP/navitia_docker_images/ubuntu_18/ubuntu18_dev/Dockerfile'
        }

      }
      steps {
        echo 'Let\'s build navitia'
        sh 'echo "Shell Script !"'
      }
    }
  }
}