<?php 
require_once 'default.inc';
class ExplainController extends DefaultController {
	private $_task = NULL;
	private $_policy = NULL;
	private $_output = NULL;
	private $_issue = NULL;
	private $_errors = array();
	protected function _init() {
		// Validate Task and User
		$taskId = $this->get ( 'id' );
		$policy = $this->get ( 'policy' );
		$output = $this->get ( 'output' );
		$issue = $this->get('issue','noissue');
		if($taskId && $policy && $output){
			$this->_task = $taskId;
			$this->_policy = $policy;
			$this->_output = $output;
			$this->_issue = $issue;
		}else{
			$this->_forbidden("No task specified.");
		}
	}
		
	public function visualAction(){
		if (! $this->_task){
			$this->_malformed ( "Missing task" );
		}
		
		$m = $this->model();
		$m['page'] = 'explain';
		$m['data']['id'] = $this->_task;
		$m['data']['policy'] = $this->_policy;
		$m['data']['output'] = $this->_output;
		$m['data']['task'] = DataAccess::instance()->task($this->_task);
		$m['data']['issue'] = $this->_issue;
		$m['title'] = $m['data']['task']->label;
		$m['data']['trace'] = DataAccess::instance()->sankey($this->_task)->trace($this->_policy, $this->_output);
		$m['data']['resolution'] = DataAccess::instance()->resolution($this->_userid(), $this->_task, $this->_policy, $this->_output);
		$v = new TaskViewer( $m );
		$v->render ( 'layout' );
	}
	
	public function resolveAction(){
		$resolutionData = $this->getAll();
		unset($resolutionData['controller']);
		unset($resolutionData['action']);
		$policy = explode(' ',$this->get('policy'));
		DataAccess::instance()->writeResolution($this->_userId(), $this->_task, $this->get('output'), $policy[0] , $policy[1], $resolutionData);
		// Back to task
		$this->_redirect('task', 'task', array('id'=>$this->_task));
		exit;
	}
}