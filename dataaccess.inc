<?php
require_once 'vendor/semsol/arc2/ARC2.php';
class DataAccess {
	private static $_instance;
	private static $_data_home = NULL;
	private static function _dataHome(){
		if(DataAccess::$_data_home == NULL){
			if($ini = parse_ini_file(__DIR__ . '/properties.ini')){
				self::$_data_home = $ini['folder.analysis'];
			}else{
				throw new Exception("Cannot parse configuration file");
			}
		}
		return self::$_data_home;
	}
	const PHASE_COMPLETE = 6;
	private $_users = array (
			1000 => 'Administrator',
			1 => 'CAAN',
			2 => 'NIFR',
			3 => 'ILAN',
			4 => 'ALPA',
			5 => 'MAPI' 
	);
	private $_users_tasks = array (
			1000 => array (
/*					'scrape-website-2',
					'lift-chart-1',
					'statistic-textual-1',
					'moving-average-1',
					'sensor-cleaning-1',
					'classifier-textual-1',*/
					'TUTORIAL',
					'CLOUD',
					'CLEAN',
					'SCRAPE',
					'AVG',
					'FOOD'
			)
			, // CAAN
			1 => array ('CLOUD','CLEAN')
			,
			// NIFR
			2 => array ('FOOD','AVG')
			,
			// ILAN
			3 => array ('SCRAPE','CLEAN')
			,
			// ALPA
			4 => array ('FOOD','CLOUD')
			,
			// MAPI
			5 => array ('AVG','SCRAPE')			 
	);
	private $_tasks = NULL;
	private $_catalogue = NULL;
	private $_rules = NULL;
	public function users() {
		return $this->_users;
	}
	public function user($userId) {
		if (array_key_exists ( $userId, $this->_users )) {
			return array (
					'id' => $userId,
					'name' => $this->_users [$userId],
					'tasks' => $this->_tasks ( $userId ) 
			);
		} else {
			return FALSE;
		}
	}
	public function userByName($user) {
		$userId = array_search ( $user, $this->_users );
		if ($userId) {
			return array (
					'id' => $userId,
					'name' => $this->_users [$userId] 
			);
		} else {
			return FALSE;
		}
	}
	public function login($username, $password) {
		$user = $this->userByName ( $username );
		if ($user !== FALSE && $password == $username) {
			return $user ['id'];
		}
		return FALSE;
	}
	public function tasks($userId = NULL) {
		if ($userId === NULL) {
			return $this->_tasks ();
		} else {
			$tasks = array ();
			foreach ( $this->_tasks () as $key => $task ) {
				if (@in_array ( $task->id, @$this->_users_tasks [$userId] )) {
					$tasks [$key] = $task;
				}
			}
			return $tasks;
		}
	}
	public function tasksUsersStatus() {
		$tus = array ();
		foreach ( $this->tasks () as $task ) {
			$tus [$task->id] = array (
					'id' => $task->id,
					'users' => array () 
			);
			foreach ( $this->users () as $usid => $uname ) {
				if ($this->userHasTask ( $usid, $task->id )) {
					$tus [$task->id] ['users'] [$usid] ['name'] = $uname;
					$tus [$task->id] ['users'] [$usid] ['status'] = $this->complete ( $usid, $task->id );
				}
			}
		}
		return $tus;
	}
	public function task($taskId) {
		foreach ( $this->_tasks () as $key => $task ) {
			if ($task->id == $taskId)
				return $task;
		}
		return NULL;
	}
	public function userHasTask($userId, $taskId) {
		if ($userId === 1000) {
			return true;
		}
		return in_array ( $taskId, $this->_users_tasks [$userId] );
	}
	public function phase($userId, $taskId, $phase = NULL) {
		$phases = $this->_loadPhases ();
		if (! is_object ( $phases )) {
			$phases = new stdClass ();
		}
		if ($phase === NULL) {
			return @$phases->{"$userId---$taskId"};
		} else {
			$phases->{"$userId---$taskId"} = $phase;
			if (! isset ( $phases->{"$userId---$taskId---visited"} )) {
				$phases->{"$userId---$taskId---visited"} = array ();
			}
			for($x = $phase; $x > 0; $x --)
				if (! in_array ( $x, $phases->{"$userId---$taskId---visited"} )) {
					array_push ( $phases->{"$userId---$taskId---visited"}, $x );
				}
			$this->_writePhases ( $phases );
			return $phase;
		}
	}
	public function phaseVisited($userId, $taskId) {
		$phases = $this->_loadPhases ();
		if (isset ( $phases->{"$userId---$taskId---visited"} )) {
			return $phases->{"$userId---$taskId---visited"};
		}
		return array ();
	}
	public function getDownloadFile($taskId) {
		$task = $this->task ( $taskId );
		return self::_dataHome() . "/data/repository/" . str_replace ( '.ttl', '.rmp', $task->dataflow );
	}
	private function _loadDataflow($taskId) {
		$task = $this->task ( $taskId );
		$turtleFile = self::_dataHome() . '/dataflows/' . $task->dataflow;
		$parser = ARC2::getTurtleParser ();
		$parser->parse ( $turtleFile );
		$triples = $parser->getTriples ();
		// mappings
		foreach ( $task->imports as $imp ) {
			$turtleFile = self::_dataHome() . '/tasks/mappings/' . $imp;
			$parser2 = ARC2::getTurtleParser ();
			$parser2->parse ( $turtleFile );
			$triples = array_merge ( $triples, $parser2->getTriples () );
		}
		return $triples;
	}
	public function sankey($taskId) {
		return new Sankey ( $this->_loadDataflow ( $taskId ) );
	}
	public function dataflows($taskId = NULL) {
		$triples = array ();
		$tids = array ();
		if ($taskId === NULL) {
			$tasks = $this->tasks ();
			foreach ( $tasks as $t ) {
				$triples = array_merge ( $triples, $this->_loadDataflow ( $t->id ) );
			}
		}
		
		$df = new TripleSet ( $triples );
		return $df;
	}
	public function datasetLabel($datasetUri) {
		foreach ( $this->_catalogue () as $trip ) {
			if ($trip ['s'] == $datasetUri && $trip ['p'] == 'http://purl.org/dc/terms/title') {
				return $trip ['o'];
			}
		}
		return substr ( $datasetUri, strrpos ( $datasetUri, "/" ) + 1 );
	}
	public function datasets($taskId) {
		$task = $this->task ( $taskId );
		// Iterate over triples and get the dataset/policies
		$datasets = array ();
		foreach ( $task->imports as $import ) {
			$turtleFile = self::_dataHome() . '/tasks/mappings/' . $import;
			$parser = ARC2::getTurtleParser ();
			$parser->parse ( $turtleFile );
			$triples = $parser->getTriples ();
			foreach ( $triples as $tr ) {
				if ($tr ['p'] === 'http://purl.org/datanode/ppr/ns/asset') {
					$datasets [$tr ['o']] = array (
							'id' => $tr ['o'],
							'terms' => $tr ['s'] 
					);
				}
			}
		}
		
		// Dataset metadata
		foreach ( $datasets as &$dt ) {
			foreach ( $this->_catalogue () as $trip ) {
				if ($trip ['s'] == $dt ['id'] && $trip ['p'] == 'http://www.w3.org/ns/dcat#landingPage') {
					$dt ['page'] = $trip ['o'];
				} elseif ($trip ['s'] == $dt ['id'] && $trip ['p'] == 'http://purl.org/dc/terms/title') {
					$dt ['title'] = $trip ['o'];
				}
				if (@$dt ['title'] && @$dt ['page']) {
					break;
				}
			}
			if (! @$dt ['title']) {
				$dt ['title'] = $dt ['id'];
				$dt ['page'] = '';
			}
		}
		
		// Policies
		foreach ( $task->policies as $polfile ) {
			$turtleFile = self::_dataHome() . '/licences/' . $polfile . '.ttl';
			$parser = ARC2::getTurtleParser ();
			$parser->parse ( $turtleFile );
			$triples = $parser->getTriples ();
			// echo "Policies: $polfile " . count($triples);
			// var_dump($triples);
			$this->_extractPolicies ( $datasets, $triples );
		}
		
		return $datasets;
	}
	public function inputs($taskId) {
		$inputs = array ();
		$task = $this->task ( $taskId );
		// Taken from the mappings
		foreach ( $task->imports as $import ) {
			$turtleFile = self::_dataHome() . '/tasks/mappings/' . $import;
			$parser = ARC2::getTurtleParser ();
			$parser->parse ( $turtleFile );
			$triples = $parser->getTriples ();
			foreach ( $triples as $tr ) {
				if (strpos ( $tr ['p'], 'http://purl.org/datanode/ns/' ) === 0) {
					array_push ( $inputs, array (
							$tr ['s'],
							$tr ['p'],
							$tr ['o'] 
					) );
				}
			}
		}
		
		return $inputs;
	}
	public function outputs($taskId) {
		$task = $this->task ( $taskId );
		// Taken from the query
		return $task->query;
	}
	public function propagatedPolicies() {
		$propagatedPolicies = array ();
		foreach ( $this->_tasks () as $task ) {
			$taskId = $task->id;
			$propagatedPolicies [$taskId] = $this->propagated ( $taskId );
		}
		return $propagatedPolicies;
	}
	public function propagated($taskId) {
		$propagated = NULL;
		
		$handle = fopen ( self::_dataHome() . "/tasks/results/suite.txt", "r" );
		if ($handle) {
			while ( ($line = fgets ( $handle )) !== false ) {
				// process the line read.
				if (strpos ( $line, '[generated/' . $taskId ) === 0) {
					$lar = explode ( "\t", $line );
					$res = $lar [7];
					$par = json_decode ( $res, true );
					// Preprocess
					foreach ( $par as $output => $arr ) {
						foreach ( $arr as $ar ) {
							$ars = explode ( ' ', $ar );
							$deontic = substr ( $ars [0], strrpos ( $ars [0], '/' ) + 1 );
							$action = $ars [1];
							$propagated [$output] [$deontic] [$action] = TRUE;
						}
					}
					break;
				}
			}
			fclose ( $handle );
		} else {
			// error opening the file.
			throw new Exception ( "Error reading file." );
		}
		
		return $propagated;
	}
	/**
	 * Compute the choices, values are the related input datasets
	 * 
	 * @param unknown $taskId        	
	 */
	public function policies($taskId) {
		$datasets = $this->datasets ( $taskId );
		$outputs = $this->outputs ( $taskId );
		$choices = array ();
		foreach ( $outputs as $output ) {
			foreach ( $datasets as $dataset ) {
				foreach ( $dataset ['policies'] as $deontic => $actions ) {
					foreach ( $actions as $action ) {
						if (! isset ( $choices [$output] [$deontic] [$action] )) {
							$choices [$output] [$deontic] [$action] = array ();
						}
						array_push ( $choices [$output] [$deontic] [$action], $dataset ['title'] );
					}
				}
			}
		}
		return $choices;
	}
	public function listPolicies() {
		$policies = array ();
		$f = __DIR__ . '/policies.nt';
		$parser = ARC2::getTurtleParser ();
		$parser->parse ( $f );
		$triples = $parser->getTriples ();
		foreach ( $triples as $tr ) {
			$d = $this->_localName ( $tr ['p'] );
			$a = $tr ['o'];
			$k = $d . ' ' . $a;
			$policies [$k] ['deontic'] = $d;
			$policies [$k] ['action'] = $a;
			$policies [$k] ['tasks'] = array ();
		}
		foreach ( $this->_tasks () as $task ) {
			$taskId = $task->id;
			$datasets = $this->datasets ( $taskId );
			$outputs = $this->outputs ( $taskId );
			foreach ( $outputs as $output ) {
				foreach ( $datasets as $dataset ) {
					foreach ( $dataset ['policies'] as $deontic => $actions ) {
						foreach ( $actions as $action ) {
							$key = $deontic . ' ' . $action;
							if (! isset ( $policies [$key] )) {
								$policies [$key] = array ();
							}
							$policies [$key] ['deontic'] = $deontic;
							$policies [$key] ['action'] = $action;
							if (! isset ( $policies [$key] ['tasks'] )) {
								$policies [$key] ['tasks'] = array ();
							}
							if (! in_array ( $taskId, $policies [$key] ['tasks'] )) {
								array_push ( $policies [$key] ['tasks'], $taskId );
							}
						}
					}
				}
			}
		}
		
		return $policies;
	}
	public function numberOfPolicies() {
		$numberOfPolicies = array ();
		foreach ( $this->_tasks () as $task ) {
			$taskId = $task->id;
			$datasets = $this->datasets ( $taskId );
			$outputs = $this->outputs ( $taskId );
			$choices = array ();
			$number = 0;
			foreach ( $outputs as $output ) {
				foreach ( $datasets as $dataset ) {
					foreach ( $dataset ['policies'] as $deontic => $actions ) {
						foreach ( $actions as $action ) {
							if (! isset ( $choices [$output] [$deontic] [$action] )) {
								$number ++;
								$choices [$output] [$deontic] [$action] = TRUE;
							}
						}
					}
				}
			}
			$numberOfPolicies [$taskId] = $number;
		}
		return $numberOfPolicies;
	}
	
	public function conflicts($userId, $taskId) {
		$choiceData = $this->choices ( $userId, $taskId );
		$propagatedData = $this->propagated ( $taskId );
		$resolutions = $this->resolutions ( $userId, $taskId );
		$conflicts = array ();
		foreach ( $choiceData as $output => $policy ) {
			foreach ( $policy as $deontic => $actions ) {
				foreach ( $actions as $action => $datasets ) {
					$status = isset ( $resolutions [$output] [$deontic] [$action] );
					if ($choiceData [$output] [$deontic] [$action] > 0 && ! isset ( $propagatedData [$output] [$deontic] [$action] )) :
						$conflicts [$output] [$deontic] [$action] = array (
								'type' => 'toolismissing',
								'status' => $status 
						);
					 elseif ($choiceData [$output] [$deontic] [$action] < 0 && isset ( $propagatedData [$output] [$deontic] [$action] )) :
						$conflicts [$output] [$deontic] [$action] = array (
								'type' => 'tooliswrong',
								'status' => $status 
						);
					 elseif ($choiceData [$output] [$deontic] [$action] == 0) :
						$conflicts [$output] [$deontic] [$action] = array (
								'type' => 'dontknow',
								'status' => $status 
						);
					endif;
				}
			}
		}
		return $conflicts;
	}
	public function complete($userId, $taskId) {
		return $this->phase ( $userId, $taskId ) == self::PHASE_COMPLETE;
	}
	public function writeResolution($userId, $taskId, $output, $deontic, $action, $data) {
		$resolutions = $this->_loadResolutions ( $userId, $taskId );
		$resolutions [$output] [$deontic] [$action] = $data;
		$this->_writeResolutions ( $userId, $taskId, $resolutions );
	}
	public function resolution($userId, $taskId, $policy, $output) {
		$p = explode ( ' ', $policy );
		$resolutions = $this->resolutions ( $userId, $taskId );
		if (isset ( $resolutions [$output] [$p [0]] [$p [1]] )) {
			return $resolutions [$output] [$p [0]] [$p [1]];
		}
		return FALSE;
	}
	public function actinfo($action) {

		$info = array (
				'id' => $action,
				'comment' => '',
				'note' => '',
				'description' => '',
				'label' => '',
				'inv' => '',
				'super' => array (),
				'sub' => array (),
				'lbls' => array ()
		);
		
		
		$f = NULL;
		if (strpos ( $action, 'http://www.w3.org/ns/odrl/2/' ) === 0) {
			$f = __DIR__ . '/odrl.ttl';
			$parser = ARC2::getTurtleParser ();
		} elseif (strpos ( $action, 'http://creativecommons.org/ns#' ) === 0) {
			$f = __DIR__ . '/cc.rdf';
			$parser = ARC2::getRDFXMLParser ();
		} elseif (strpos ( $action, 'http://purl.org/NET/ldr/ns#' ) === 0) {
			$f = __DIR__ . '/ldr.rdf';
			$parser = ARC2::getRDFXMLParser ();
		}
		if($f === NULL) return $info;
		
		$parser->parse ( $f );
		$triples = $parser->getTriples ();
		foreach ( $triples as $tr ) {
			if (strpos ( $tr ['s'], $action ) === 0) {
				switch ($tr ['p']) {
					case 'http://www.w3.org/2004/02/skos/core#note' :
						$info ['note'] = $tr ['o'];
						break;
					case 'http://www.w3.org/2000/01/rdf-schema#comment' :
						$info ['comment'] = $tr ['o'];
						break;
					case 'http://purl.org/dc/terms/description' :
						$info ['description'] = $tr ['o'];
						break;
				}
			}
			if (strpos ( $tr ['s'], $action ) === 0) {
				switch ($tr ['p']) {
					case 'http://www.w3.org/2000/01/rdf-schema#subPropertyOf' :
						array_push ( $info ['super'], $tr ['o'] );
						break;
				}
			} else if (strpos ( $tr ['o'], $action ) === 0) {
				switch ($tr ['p']) {
					case 'http://www.w3.org/2000/01/rdf-schema#subPropertyOf' :
						array_push ( $info ['sub'], $tr ['s'] );
						break;
				}
			}
			if ($tr ['p'] == 'http://www.w3.org/2002/07/owl#inverseOf') {
				if ($tr ['s'] == $action) {
					$info ['inv'] = $tr ['o'];
				}
				if ($tr ['o'] == $action) {
					$info ['inv'] = $tr ['s'];
				}
			}
		}
		
		foreach ( $triples as $tr ) {
			if ($tr ['p'] == 'http://www.w3.org/2000/01/rdf-schema#label') {
				if ($tr ['s'] == $action) {
					$info ['label'] = $tr ['o'];
				}
				if (in_array ( $tr ['s'], $info ['sub'] ) || in_array ( $tr ['s'], $info ['super'] ) || $tr ['s'] == $info ['inv']) {
					$info ['lbls'] [$tr ['s']] = $tr ['o'];
				}
			}
		}
		
		return $info;
	}
	
	public function dninfo($relation) {
		$dnns = 'http://purl.org/datanode/ns/';
		$f = __DIR__ . '/datanode-doc.ttl';
		$parser = ARC2::getTurtleParser ();
		$parser->parse ( $f );
		$triples = $parser->getTriples ();
		$info = array (
				'id' => $relation,
				'comment' => '',
				'note' => '',
				'description' => '',
				'label' => '',
				'inv' => '',
				'super' => array (),
				'sub' => array (),
				'lbls' => array () 
		);
		$info ['id'] = $relation;
		foreach ( $triples as $tr ) {
			if (strpos ( $tr ['s'], 'http://purl.org/datanode/ns/' . $relation ) === 0) {
				switch ($tr ['p']) {
					case 'http://www.w3.org/2004/02/skos/core#note' :
						$info ['note'] = $tr ['o'];
						break;
					case 'http://www.w3.org/2000/01/rdf-schema#comment' :
						$info ['comment'] = $tr ['o'];
						break;
					case 'http://purl.org/dc/terms/description' :
						$info ['description'] = $tr ['o'];
						break;
				}
			}
		}
		
		$f = __DIR__ . '/datanode-sub.ttl';
		$parser = ARC2::getTurtleParser ();
		$parser->parse ( $f );
		$triples = $parser->getTriples ();
		foreach ( $triples as $tr ) {
			if (strpos ( $tr ['s'], 'http://purl.org/datanode/ns/' . $relation ) === 0) {
				switch ($tr ['p']) {
					case 'http://www.w3.org/2000/01/rdf-schema#subPropertyOf' :
						array_push ( $info ['super'], $tr ['o'] );
						break;
				}
			}
			if (strpos ( $tr ['o'], 'http://purl.org/datanode/ns/' . $relation ) === 0) {
				switch ($tr ['p']) {
					case 'http://www.w3.org/2000/01/rdf-schema#subPropertyOf' :
						array_push ( $info ['sub'], $tr ['s'] );
						break;
				}
			}
		}
		
		$f = __DIR__ . '/datanode-inv.ttl';
		$parser = ARC2::getTurtleParser ();
		$parser->parse ( $f );
		$triples = $parser->getTriples ();
		foreach ( $triples as $tr ) {
			if ($tr ['p'] == 'http://www.w3.org/2002/07/owl#inverseOf') {
				if ($tr ['s'] == 'http://purl.org/datanode/ns/' . $relation) {
					$info ['inv'] = $tr ['o'];
				}
				if ($tr ['o'] == 'http://purl.org/datanode/ns/' . $relation) {
					$info ['inv'] = $tr ['s'];
				}
			}
		}
		
		$f = __DIR__ . '/datanode-lbl.ttl';
		$parser = ARC2::getTurtleParser ();
		$parser->parse ( $f );
		$triples = $parser->getTriples ();
		foreach ( $triples as $tr ) {
			if ($tr ['p'] == 'http://www.w3.org/2000/01/rdf-schema#label') {
				if ($tr ['s'] == 'http://purl.org/datanode/ns/' . $relation) {
					$info ['label'] = $tr ['o'];
				}
				if (in_array ( $tr ['s'], $info ['sub'] ) || in_array ( $tr ['s'], $info ['super'] ) || $tr ['s'] == $info ['inv']) {
					$info ['lbls'] [$tr ['s']] = $tr ['o'];
				}
			}
		}
		
		return $info;
	}
	
	public function resultsSummary($deonticFilter = NULL){
		// By task, tool/group1, tool/group2, group1/group2
		$results = array ();
		// Cycle through tasks
		foreach ( $this->tasksUsersStatus () as $tid => $tus ) {
			foreach ( $tus ['users'] as $use ) {
				// if($use['status'] == 6){
				$j = array (
					'task' => $tid,
					'team' => $use ['name']
				);
				// Collect the conflicts
				$uid = $this->userByName ( $j ['team'] );
				$uid = $uid ['id'];
				if ($uid == 1000)
					continue;
				
				// For each task
				if(!isset($results[$tid])){
					$results[$tid] = array('taskId'=>$tid,'totalDecisions'=>$this->totalDecisions($tid,$deonticFilter));
				}
				if(!isset($results[$tid]['users'])){
					$results[$tid]['users'] = array($use['name']);
				}else{
					array_push($results[$tid]['users'],$use['name']);
					$user1 = $this->userByName($results[$tid]['users'][0]);
					$user2 = $this->userByName($results[$tid]['users'][1]);
					// Compute agreed with system
					$results[$tid]['team1'] = $this->computeAgreed($tid, $user1['id'],$deonticFilter);
					$results[$tid]['team2'] = $this->computeAgreed($tid, $user2['id'],$deonticFilter);
					// Strong decisions
					$results[$tid]['team1-strong'] = $this->computeStrongDecisions($tid, $user1['id'],$deonticFilter);
					$results[$tid]['team2-strong'] = $this->computeStrongDecisions($tid, $user2['id'],$deonticFilter);
					// Compute agreed with each other
					$results[$tid]['agreed'] = $this->computeAgreementBetweenTeams($tid, $user1['id'], $user2['id'],$deonticFilter);
					$results[$tid]['full-agreed'] = $this->computeFullAgreementBetweenTeams($tid, $user1['id'], $user2['id'],$deonticFilter);
				}
			}
		}
		
		return $results;
	}

	public function computeAgreementBetweenTeams($taskId, $team1Id, $team2Id, $deonticFilter = NULL){
		$choiceData1 = $this->choices ( $team1Id, $taskId );
		$choiceData2 = $this->choices ( $team2Id, $taskId );
		$agreed = 0;
		foreach ( $choiceData1 as $output => $policy ) {
			foreach ( $policy as $deontic => $actions ) {
				if($deonticFilter !=NULL && $deonticFilter!=$deontic) continue;
				foreach ( $actions as $action => $datasets ) {
					if ($choiceData1 [$output] [$deontic] [$action] > 0 && $choiceData2 [$output] [$deontic] [$action] > 0) :
					$agreed++;
					elseif ($choiceData1 [$output] [$deontic] [$action] < 0 && $choiceData2 [$output] [$deontic] [$action] < 0 ) :
					$agreed++;
					elseif ($choiceData1 [$output] [$deontic] [$action] == 0 && $choiceData2 [$output] [$deontic] [$action] == 0) :
					$agreed++;
					else :
					// Disagreeing
					endif;
				}
			}
		}
		return $agreed;
	}
	public function computeFullAgreementBetweenTeams($taskId, $team1Id, $team2Id,$deonticFilter=NULL){
		$choiceData1 = $this->choices ( $team1Id, $taskId );
		$choiceData2 = $this->choices ( $team2Id, $taskId );
		$agreed = 0;
		foreach ( $choiceData1 as $output => $policy ) {
			foreach ( $policy as $deontic => $actions ) {
				if($deonticFilter!=NULL && $deonticFilter!= $deontic) continue;
				foreach ( $actions as $action => $datasets ) {
					if ($choiceData1 [$output] [$deontic] [$action] == $choiceData2 [$output] [$deontic] [$action]) :
					$agreed++;
					endif;
				}
			}
		}
		return $agreed;
	}
	
	public function totalDecisions($taskId, $deonticFilter = NULL) {
		$c=0;
		foreach ( $this->policies ( $taskId ) as $output => $policy ) {
			foreach ( $policy as $deontic => $actions ) {
				if($deonticFilter != NULL && $deonticFilter != $deontic) continue;
				foreach ( $actions as $action => $datasets ) {
					$c++;
				}
			}
		}
		return $c;
	}

	public function computeAgreed($taskId, $userId, $deonticFilter = NULL){
		// 		var_dump($userId);die;
		$choiceData = $this->choices ( $userId, $taskId );
		$propagatedData = $this->propagated ( $taskId );
		$agreed = 0;
		foreach ( $choiceData as $output => $policy ) {
			foreach ( $policy as $deontic => $actions ) {
				if($deonticFilter!=NULL && $deonticFilter!= $deontic) continue;
				foreach ( $actions as $action => $datasets ) {
					if ($choiceData [$output] [$deontic] [$action] > 0 && ! isset ( $propagatedData [$output] [$deontic] [$action] )) :
					// still disagreeing toolismissing
					elseif ($choiceData [$output] [$deontic] [$action] < 0 && isset ( $propagatedData [$output] [$deontic] [$action] )) :
					// still disagreeing tooliswrong
					elseif ($choiceData [$output] [$deontic] [$action] == 0) :
					// still disagreeing dontknow
					else :
					// Agreeing
					$agreed++;
					endif;
				}
			}
		}
		return $agreed;
	}
	public function computeStrongDecisions($taskId, $userId, $deonticFilter = NULL){
// 		var_dump($userId);die;
		$choiceData = $this->choices ( $userId, $taskId );
		$agreed = 0;
		foreach ( $choiceData as $output => $policy ) {
			foreach ( $policy as $deontic => $actions ) {
				if($deonticFilter!=NULL && $deonticFilter!= $deontic) continue;
				foreach ( $actions as $action => $datasets ) {
					if ($choiceData [$output] [$deontic] [$action] == -2 || $choiceData [$output] [$deontic] [$action] == 2) :
					$agreed++;
					endif;
				}
			}
		}
		return $agreed;
	}
	public function resolutionsSummary($tidFilter = NULL, $userFilter = NULL) {
		$conflicts = array ();
		// Cycle through tasks
		foreach ( $this->tasksUsersStatus () as $tid => $tus ) {
			if($tidFilter!=NULL && $tidFilter!=$tid) continue;
			// Skip Administrator
			foreach ( $tus ['users'] as $use ) {
				// if($use['status'] == 6){
				$j = array (
						'task' => $tid,
						'team' => $use ['name'] 
				);
				if($userFilter!=NULL && $userFilter!=$use['name']) continue;
				// Collect the conflicts
				$uid = $this->userByName ( $j ['team'] );
				$uid = $uid ['id'];
				if ($uid == 1000)
					continue;
				
				$userId = $uid;
				$taskId = $tid;
				$choiceData = $this->choices ( $userId, $taskId );
				$propagatedData = $this->propagated ( $taskId );
				$resolutions = $this->resolutions ( $uid, $j ['task'] );
				
				$agreedAfter = array ();
				foreach ( $choiceData as $output => $policy ) {
					foreach ( $policy as $deontic => $actions ) {
						foreach ( $actions as $action => $datasets ) {
							$disagreed = isset ( $resolutions [$output] [$deontic] [$action] );
							if ($choiceData [$output] [$deontic] [$action] > 0 && ! isset ( $propagatedData [$output] [$deontic] [$action] )) :
														// still disagreeing toolismissing
							elseif ($choiceData [$output] [$deontic] [$action] < 0 && isset ( $propagatedData [$output] [$deontic] [$action] )) :
														// still disagreeing tooliswrong
							elseif ($choiceData [$output] [$deontic] [$action] == 0) :
														// still disagreeing dontknow
							elseif ($disagreed) :
								// AGREED AFTER!
								$agreedAfter [$j ['team']] [$j ['task']] [$output] [$deontic . ' ' . $action] = $choiceData [$output] [$deontic] [$action];
							 else :
														// Not disagreeing
							endif;
						}
					}
				}
				foreach ( $resolutions as $output => $m ) {
					foreach ( $m as $deo => $mm ) {
						foreach ( $mm as $act => $detail ) {
							$conflict = $detail;
							$conflict ['team'] = $j ['team'];
							$conflict ['task'] = $j ['task'];
							if (isset ( $agreedAfter [$j ['team']] [$j ['task']] [$output] [$deo . ' ' . $act] )) {
								
								$conflict ['agreedAfter'] = 'T';
							} else {
								$conflict ['agreedAfter'] = 'F';
							}
							// Check whether they changed their mind
							array_push ( $conflicts, $conflict );
						}
					}
				}
			}
		}
		return $conflicts;
	}
	
	public function relations() {
		$dnns = 'http://purl.org/datanode/ns/';
		
		$relations = array ();
		$f = __DIR__ . '/datanode-sub.ttl';
		$parser = ARC2::getTurtleParser ();
		$parser->parse ( $f );
		$triples = $parser->getTriples ();
		foreach ( $triples as $tr ) {
			if ($tr ['p'] == 'http://www.w3.org/2000/01/rdf-schema#subPropertyOf') {
				$relations [$tr ['o']] = '';
				$relations [$tr ['s']] = '';
			}
		}
		
		$f = __DIR__ . '/datanode-doc.ttl';
		$parser = ARC2::getTurtleParser ();
		$parser->parse ( $f );
		$triples = $parser->getTriples ();
		
		$f = __DIR__ . '/datanode-lbl.ttl';
		$parser = ARC2::getTurtleParser ();
		$parser->parse ( $f );
		$labels = $parser->getTriples ();
		$triples = array_merge ( $triples, $labels );
		
		foreach ( $triples as $tr ) {
			if (isset ( $relations [$tr ['s']] )) {
				switch ($tr ['p']) {
					case 'http://www.w3.org/2000/01/rdf-schema#label' :
						$relations [$tr ['s']] = $tr ['o'];
						break;
				}
			}
		}
		return $relations;
	}
	public function subproperties($relations = array()) {
		$f = __DIR__ . '/datanode-sub.ttl';
		$parser = ARC2::getTurtleParser ();
		$parser->parse ( $f );
		$triples = $parser->getTriples ();
		
		if (empty ( $relations )) {
			$relations = $this->relations ();
		} else if (is_string ( $relations )) {
			$relations = array (
					$relations 
			);
		}
		$tset = new TripleSet ( $triples );
		$subs = array ();
		foreach ( $relations as $rel ) {
			$subs [$rel] = $tset->transitiveClosure ( $rel, 'http://www.w3.org/2000/01/rdf-schema#subPropertyOf', 0 );
		}
		return $subs;
	}
	public function inverses($relations = array()) {
		$f = __DIR__ . '/datanode-inv.ttl';
		$parser = ARC2::getTurtleParser ();
		$parser->parse ( $f );
		$triples = $parser->getTriples ();
		
		if (empty ( $relations )) {
			$relations = $this->relations ();
		} else if (is_string ( $relations )) {
			$relations = array (
					$relations 
			);
		}
		$tset = new TripleSet ( $triples );
		$subs = array ();
		foreach ( $relations as $rel ) {
			$i = $tset->inverseOf ( $rel );
			$subs [$rel] = $i;
			$subs [$i] = $rel;
		}
		return $subs;
	}
	public function resolutions($userId, $taskId) {
		return $this->_loadResolutions ( $userId, $taskId );
	}
	private function _loadResolutions($userId, $taskId) {
		$f = __DIR__ . '/state/' . $taskId . '__' . $userId . '--resolutions.json';
		if (! file_exists ( $f )) {
			return array ();
		}
		return json_decode ( file_get_contents ( $f ), true );
	}
	private function _writeResolutions($userId, $taskId, $resolutions) {
		$f = __DIR__ . '/state/' . $taskId . '__' . $userId . '--resolutions.json';
		return file_put_contents ( $f, json_encode ( $resolutions ) );
	}
	public function choices($userId, $taskId, $choices = NULL) {
		if ($choices === NULL) {
			if($ret = $this->_loadChoices ( $userId, $taskId )){
				return $ret;
			}
			return array();
		} else {
			$this->_writeChoices ( $userId, $taskId, $choices );
		}
	}
	private function _loadChoices($userId, $taskId) {
		$f = __DIR__ . '/state/' . $taskId . '__' . $userId . '.json';
		if (! file_exists ( $f )) {
			return NULL;
		}
		return json_decode ( file_get_contents ( $f ), true );
	}
	private function _writeChoices($userId, $taskId, $choices) {
		return file_put_contents ( __DIR__ . '/state/' . $taskId . '__' . $userId . '.json', json_encode ( $choices ) );
	}
	private function _extractPolicies(&$datasets, &$triples, &$policies = NULL, $bnode = NULL) {
		if ($policies == NULL) {
			$policies = array (
					'permission' => array (),
					'prohibition' => array (),
					'duty' => array () 
			);
		}
		if ($bnode !== NULL) {
			$deontic = NULL;
			$actions = array ();
			foreach ( $triples as $tr ) {
				// search for bnode policies
				if ($tr ['o'] === $bnode) {
					$deontic = str_replace ( 'http://www.w3.org/ns/odrl/2/', '', $tr ['p'] );
				} elseif ($tr ['s'] === $bnode && $tr ['p'] == 'http://www.w3.org/ns/odrl/2/action') {
					array_push ( $actions, $tr ['o'] );
				}
			}
			if (! isset ( $policies [$deontic] )) {
				$policies [$deontic] = array ();
			}
			$policies [$deontic] = array_merge ( $policies [$deontic], $actions );
		} else {
			foreach ( $triples as $tr ) {
				if ($tr ['o'] === 'http://www.w3.org/ns/odrl/2/Policy') {
					// Bind with dataset as configuration of the task uses file name ...
					foreach ( $datasets as &$d ) {
						if ($d ['terms'] === $tr ['s']) {
							$d ['policies'] = &$policies;
						}
					}
				} elseif ((in_array ( $tr ['p'], array (
						'http://www.w3.org/ns/odrl/2/prohibition',
						'http://www.w3.org/ns/odrl/2/permission',
						'http://www.w3.org/ns/odrl/2/duty' 
				) ))) {
					if (strpos ( $tr ['o'], '_:' ) === 0) {
						// bnode
						$this->_extractPolicies ( $datasets, $triples, $policies, $tr ['o'] );
					} else {
						array_push ( $policies [str_replace ( 'http://www.w3.org/ns/odrl/2/', '', $tr ['p'] )], $tr ['o'] );
					}
				}
			}
		}
	}
	private function _loadCatalogue() {
		$parser = ARC2::getTurtleParser ();
		$parser->parse ( self::_dataHome() . '/catalogue/datasets.ttl' );
		$this->_catalogue = $parser->getTriples ();
	}
	private function _loadRules() {
		$parser = ARC2::getTurtleParser ();
		$parser->parse ( self::_dataHome() . '/tasks/kb2/full-rules-cALL.nt' );
		$this->_rules = new Rules ( $parser->getTriples () );
	}
	public function rules() {
		if ($this->_rules === NULL) {
			$this->_loadRules ();
		}
		return $this->_rules;
	}
	private function _catalogue() {
		if ($this->_catalogue === NULL) {
			$this->_loadCatalogue ();
		}
		return $this->_catalogue;
	}
	private function _tasks() {
		if ($this->_tasks === NULL) {
			$this->_loadTasks ();
		}
		return $this->_tasks;
	}
	private function _loadTasks() {
		$tasks = json_decode ( file_get_contents ( self::_dataHome() . '/tasks/tasks.json' ) );
		$this->_tasks = array ();
		foreach ( $tasks as $task ) {
			// Filter out disabled tasks
			if (! @$task->disabled && @$task->id !== NULL) {
				array_push ( $this->_tasks, $task );
			}
		}
	}
	private function _loadPhases() {
		return json_decode ( file_get_contents ( __DIR__ . '/state/phases.json' ) );
	}
	private function _writePhases($phases) {
		return json_decode ( file_put_contents ( __DIR__ . '/state/phases.json', json_encode ( $phases ) ) );
	}
	private function _localName($s) {
		if (strpos ( $s, '#' )) {
			$s = substr ( $s, strpos ( $s, '#' ) + 1 );
		} elseif (strpos ( $s, '_' ) === 0) {
			$s = substr ( $s, 2 );
		} else {
			$s = substr ( $s, strrpos ( $s, '/' ) + 1 );
		}
		return $s;
	}
	public static function instance() {
		if (self::$_instance === NULL) {
			self::$_instance = new DataAccess ();
		}
		return self::$_instance;
	}
}
class Rules {
	private $_triples;
	public function __construct($triples) {
		$this->_triples = $triples;
		$this->_index = ARC2::getSimpleIndex ( $triples, false );
	}
	public function propagates($relation, $policy) {
		$policy = explode ( ' ', 'http://www.w3.org/ns/odrl/2/' . $policy );
		$deontic = $policy [0];
		$action = $policy [1];
		if (! isset ( $this->_index [$relation] ))
			return false;
		
		foreach ( $this->_index [$relation] ['http://purl.org/datanode/ppr/ns/propagates'] as $p ) {
			$policyId = $p ['value'];
			if (isset ( $this->_index [$policyId] [$deontic] )) {
				foreach ( @$this->_index [$policyId] [$deontic] as $act ) {
					if ($act ['value'] == $action) {
						return true;
					}
				}
			}
		}
		return false;
	}
	public function numberOfRules($relation = NULL, $action = NULL) {
		if (is_array ( $relation )) {
			// nothing to do
		} else if ($relation === NULL) {
			$relation = array_keys ( $this->_index );
		} else {
			$relation = array (
					$relation 
			);
		}
		$number = array ();
		foreach ( $relation as $rel ) {
			$number [$rel] = 0;
			if (isset ( $this->_index [$rel] ['http://purl.org/datanode/ppr/ns/propagates'] ))
				foreach ( $this->_index [$rel] ['http://purl.org/datanode/ppr/ns/propagates'] as $p ) {
					if ($action === NULL) {
						$number [$rel] ++;
						continue;
					}
					$policyId = $p ['value'];
					if (isset ( $this->_index [$policyId] [$deontic] )) {
						foreach ( @$this->_index [$policyId] [$deontic] as $act ) {
							if ($act ['value'] == $action) {
								$number [$rel] ++;
							}
						}
					}
				}
		}
		return $number;
	}
}
class TripleSet {
	private $_triples;
	public function __construct($triples) {
		$this->_triples = $triples;
	}
	public function triplesWith($s = NULL, $p = NULL, $o = NULL) {
		$c = 0;
		foreach ( $this->_triples as $tr ) {
			
			$ok = TRUE;
			if ($s !== NULL && $s != $tr ['s']) {
				$ok = FALSE;
			}
			if ($ok && $p !== NULL && $p != $tr ['p']) {
				$ok = FALSE;
			}
			if ($ok && $o !== NULL && $o != $tr ['o']) {
				$ok = FALSE;
			}
			if ($ok) {
				$c ++;
			}
		}
		return $c;
	}
	public function mentions($s = NULL, $p = NULL, $o = NULL) {
		foreach ( $this->_triples as $tr ) {
			$ok = TRUE;
			if ($s !== NULL && $s != $tr ['s']) {
				$ok = FALSE;
			}
			if ($ok && $p !== NULL && $p != $tr ['p']) {
				$ok = FALSE;
			}
			if ($ok && $o !== NULL && $o != $tr ['o']) {
				$ok = FALSE;
			}
			if ($ok) {
				return TRUE;
			}
		}
		return FALSE;
	}
	public function transitiveClosure($node, $follow, $direction = 1) {
		$return = array ();
		$this->_closure ( $node, $follow, $return, $direction );
		return $return;
	}
	private function _closure($current, $follow, &$store = array(), $direction = 1) {
		foreach ( $this->_triples as $tr ) {
			if ($direction) {
				// forward
				$from = $tr ['s'];
				$to = $tr ['o'];
			} else {
				// backwords
				$from = $tr ['o'];
				$to = $tr ['s'];
			}
			if ($from == $current && $tr ['p'] == $follow) {
				array_push ( $store, $to );
				$this->_closure ( $to, $follow, $store, $direction );
			}
		}
	}
	public function inverseOf($subject) {
		$iof = 'http://www.w3.org/2002/07/owl#inverseOf';
		foreach ( $this->_triples as $tr ) {
			if ($tr ['s'] == $subject && $tr ['p'] == $iof) {
				return $tr ['o'];
			}
		}
	}
	
}
class Sankey {
	private $_triples;
	private $_index;
	private $_inverted;
	public function __construct($triples) {
		$this->_triples = $triples;
		$this->_index = ARC2::getSimpleIndex ( $triples, false );
		$this->_buildinverted ();
	}
	private function _buildInverted() {
		$inverted = array ();
		foreach ( $this->_index as $s => $po ) {
			foreach ( $po as $p => $oarr ) {
				foreach ( $oarr as $o ) {
					if ($o ['type'] == 'uri' || $o ['type'] == 'bnode') {
						if (! isset ( $inverted [$o ['value']] )) {
							$inverted [$o ['value']] = array ();
						}
						if (! isset ( $inverted [$o ['value']] [$p] )) {
							$inverted [$o ['value']] [$p] = array ();
						}
						array_push ( $inverted [$o ['value']] [$p], array (
								'value' => $s,
								'type' => 'uri' 
						) );
					}
				}
			}
		}
		$this->_inverted = $inverted;
	}
	private function _name($s) {
		if (strpos ( $s, '#' )) {
			$s = substr ( $s, strpos ( $s, '#' ) + 1 );
		} elseif (strpos ( $s, '_' ) === 0) {
			$s = substr ( $s, 2 );
		} elseif (strpos ( $s, "http://datahub.mksmart.org/ns/dataset/" ) === 0) {
			$s = DataAccess::instance ()->datasetLabel ( $s );
			$s = preg_replace ( '/[^a-z0-9]/i', ' ', $s );
		}
		$s = str_replace ( ' ', '-', $s );
		return $s;
	}
	public function process() {
		$sankey = array ();
		$sankey ['nodes'] = array ();
		$sankey ['links'] = array ();
		$nodes = array (); // Index of nodes
		$found = array (); // Index of nodes
		$dnns = 'http://purl.org/datanode/ns/';
		$links = array ();
		foreach ( $this->_triples as $tr ) {
			
			$s = $tr ['s'];
			$p = $tr ['p'];
			$o = $tr ['o'];
			if (strpos ( $p, 'http://purl.org/datanode/ns/' ) !== 0)
				continue;
			
			$sname = $this->_name ( $s );
			$oname = $this->_name ( $o );
			if (! $sname || ! $oname) {
				continue;
			}
			// Prepare nodes
			if (! in_array ( $sname, $found )) {
				$n = new stdClass ();
				$n->name = $sname;
				array_push ( $nodes, $n );
				array_push ( $found, $sname );
			}
			if (! in_array ( $oname, $found )) {
				$n = new stdClass ();
				$n->name = $oname;
				array_push ( $nodes, $n );
				array_push ( $found, $oname );
			}
			$six = array_search ( $sname, $found );
			$oix = array_search ( $oname, $found );
			$lid = $six . '->' . $oix;
			if (! isset ( $links [$lid] )) {
				$link = new stdClass ();
				$link->source = $six;
				$link->target = $oix;
				$link->value = 0;
				$link->name = '';
				$link->propagates = FALSE;
			} else {
				$link = $links [$lid];
			}
			
			$pname = substr ( $p, strrpos ( $p, '/' ) + 1 );
			// If pname already there, continue
			if (strpos ( $link->name, $pname ) !== FALSE) {
				continue;
			}
			$link->value = ($link->value + 1);
			$link->name .= (($link->name) ? ',' : '') . $pname;
			$links [$lid] = $link;
		}
		$sankey ['nodes'] = $nodes;
		$sankey ['links'] = array_values ( $links );
		return $sankey;
	}
	public function trace($policy, $output, $propagating = NULL) {
		$sankey = array ();
		$sankey ['nodes'] = array ();
		$sankey ['links'] = array ();
		$found = array (); // Index of nodes
		$dnns = 'http://purl.org/datanode/ns/';
		$seek = array (
				$output 
		);
		$rules = DataAccess::instance ()->rules ();
		$links = array ();
		$inputs = array ();
		$pprs = 0;
		// Paths from output and link index
		while ( ! empty ( $seek ) ) {
			$o = array_shift ( $seek );
			
			if (! isset ( $this->_inverted [$o] )) {
				// Beginning reached...
				continue;
			}
			
			// Get all Incoming Links
			foreach ( $this->_inverted [$o] as $p => $subjects ) {
				// If P not datanode, skip
				if (strpos ( $p, 'http://purl.org/datanode/ns/' ) !== 0)
					continue;
				foreach ( $subjects as $s ) {
					$s = $s ['value'];
					// If subject is a dataset URI, then this is an INPUT!
					$sisInput = FALSE;
					if (strpos ( $s, "http://datahub.mksmart.org/ns/dataset/" ) === 0) {
						$sisInput = TRUE;
					}
					$oisInput = FALSE;
					if (strpos ( $o, "http://datahub.mksmart.org/ns/dataset/" ) === 0) {
						$oisInput = TRUE;
					}
					
					if ($propagating != NULL) {
						if ($propagating && ! $rules->propagates ( $p, $policy )) {
							continue;
						}
					}
					
					$sname = $this->_name ( $s );
					$oname = $this->_name ( $o );
					if (! $sname || ! $oname) {
						continue;
					}
					
					// Prepare nodes
					if (! in_array ( $sname, $found )) {
						array_push ( $found, $sname );
						array_push ( $seek, $s ); // proceed
						if ($sisInput)
							array_push ( $inputs, $s );
					}
					if (! in_array ( $oname, $found )) {
						array_push ( $found, $oname );
						if ($oisInput)
							array_push ( $inputs, $o );
					}
					
					// Link
					$six = array_search ( $sname, $found );
					$oix = array_search ( $oname, $found );
					$lid = $six . '->' . $oix;
					if (! isset ( $links [$lid] )) {
						$link = new stdClass ();
						$link->source = $six;
						$link->target = $oix;
						$link->value = 0;
						$link->name = '';
						$link->propagates = FALSE;
					} else {
						$link = $links [$lid];
					}
					
					$pname = substr ( $p, strrpos ( $p, '/' ) + 1 );
					// If pname already there, continue
					if (strpos ( $link->name, $pname ) !== FALSE) {
						continue;
					}
					$link->value = ($link->value + 1);
					$link->name .= (($link->name) ? ',' : '') . $pname;
					if ($policy) {
						$ppr = DataAccess::instance ()->rules ()->propagates ( $p, $policy );
						$link->ppr [$pname] = $ppr;
						if (! $link->propagates) {
							$link->propagates = $ppr;
						}
					}
					$pprs ++;
					$links [$lid] = $link;
				}
			}
		}
		
		// Collect paths form inputs
		$inputs = array_unique ( $inputs );
		$inlinks = array ();
		$found2 = array ();
		$seek = $inputs;
		while ( ! empty ( $seek ) ) {
			$s = array_shift ( $seek );
			if (array_key_exists ( $s, $this->_index )) {
				foreach ( $this->_index [$s] as $p => $oss ) {
					foreach ( $oss as $o ) {
						
						$o = $o ['value'];
						$sname = $this->_name ( $s );
						$oname = $this->_name ( $o );
						// Link
						$six = array_search ( $sname, $found );
						$oix = array_search ( $oname, $found );
						
						if ($oix === FALSE) {
							continue;
						}
						$lid = $six . '->' . $oix;
						array_push ( $inlinks, $lid );
						
						if (! in_array ( $o, $found2 )) {
							array_push ( $seek, $o ); // proceed
							array_push ( $found2, $o );
						}
					}
				}
			}
		}
		// Keep only common links (skip paths to non-input origins)
		$links = array_intersect_key ( $links, array_flip ( $inlinks ) );
		$nodes = array ();
		$index = array ();
		// Now populate nodes array rewriting link target and source references
		foreach ( $links as $lid => &$link ) {
			// Lookup original index
			$sname = $found [$link->source];
			$oname = $found [$link->target];
			
			if (! in_array ( $sname, $index )) {
				$n = new stdClass ();
				$n->name = $sname;
				array_push ( $nodes, $n );
				array_push ( $index, $sname );
			}
			if (! in_array ( $oname, $index )) {
				$n = new stdClass ();
				$n->name = $oname;
				array_push ( $nodes, $n );
				array_push ( $index, $oname );
			}
			// Fix indexes
			$link->source = array_search ( $sname, $index );
			$link->target = array_search ( $oname, $index );
		}
		
		// Set links
		$sankey ['nodes'] = $nodes;
		$sankey ['links'] = array_values ( $links );
		$sankey ['relcount'] = $pprs;
		return $sankey;
	}
}
