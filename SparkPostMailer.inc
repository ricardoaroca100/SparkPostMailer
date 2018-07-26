<?php

class SparkpostMailer
{


    public $API_KEY = '';
    public $API_URL = 'https://api.sparkpost.com/api/v1/transmissions';

    public $From              = 'root@localhost';
    public $FromName          = 'Root User';
    public $Sender            = '';

    public $Subject           = '';
    public $Body              = '';
    public $AltBody           = '';

    private   $to             = array();
    private   $cc             = array();
    private   $bcc            = array();
    private   $ReplyTo        = array();
    private   $all_recipients = array();
    private   $attachment     = array();

    public $result;
    public $debug = 0;
    public $debug_text = '';


    public function __construct()
    {
    }
    public function IsSMTP()
    {
    }

    public function AddAddress($address, $name = '') {
        return $this->AddAnAddress('to', $address, $name);
    }
    public function AddCC($address, $name = '') {
        return $this->AddAnAddress('cc', $address, $name);
    }
    public function AddBCC($address, $name = '') {
        return $this->AddAnAddress('bcc', $address, $name);
    }
    public function AddReplyTo($address, $name = '') {
        return $this->AddAnAddress('ReplyTo', $address, $name);
    }

    public function SetFrom($address, $name = '', $auto=1) {
        $address = trim($address);
        $name = trim(preg_replace('/[\r\n]+/', '', $name)); //Strip breaks and trim
        if (!self::ValidateAddress($address)) {

            return false;
        }
        $this->From = $address;
        $this->FromName = $name;
        if ($auto) {
            if (empty($this->ReplyTo)) {
                $this->AddAnAddress('ReplyTo', $address, $name);
            }
            if (empty($this->Sender)) {
                $this->Sender = $address;
            }
        }
        return true;
    }

    public function AddAttachment($path, $name = '', $encoding = 'base64', $type = 'application/octet-stream') {

        $filename = basename($path);
        if ( $name == '' ) {
            $name = $filename;
        }

        $this->attachment[] = array(
            0 => $path,
            1 => $filename,
            2 => $name,
            3 => $encoding,
            4 => $type,
            5 => false,  // isStringAttachment
            6 => 'attachment',
            7 => 0
        );

        return true;
    }


    /**
     * Adds an address to one of the recipient arrays
     * Addresses that have been added already return false, but do not throw exceptions
     * @param string $kind One of 'to', 'cc', 'bcc', 'ReplyTo'
     * @param string $address The email address to send to
     * @param string $name
     * @return boolean true on success, false if address already used or invalid in some way
     * @access private
     */
    private function AddAnAddress($kind, $address, $name = '') {
        if (!preg_match('/^(to|cc|bcc|ReplyTo)$/', $kind)) {
            //echo 'Invalid recipient array: ' . kind;
            return false;
        }
        $address = trim($address);
        $name = trim(preg_replace('/[\r\n]+/', '', $name)); //Strip breaks and trim
        if (!self::ValidateAddress($address)) {
            return false;
        }
        if ($kind != 'ReplyTo') {
            if (!isset($this->all_recipients[strtolower($address)])) {
                array_push($this->$kind, array($address, $name));
                $this->all_recipients[strtolower($address)] = true;
                return true;
            }
        } else {
            if (!array_key_exists(strtolower($address), $this->ReplyTo)) {
                $this->ReplyTo[strtolower($address)] = array($address, $name);
                return true;
            }
        }
        return false;
    }

    /**
     * Check that a string looks roughly like an email address should
     * Static so it can be used without instantiation
     * Tries to use PHP built-in validator in the filter extension (from PHP 5.2), falls back to a reasonably competent regex validator
     * Conforms approximately to RFC2822
     * @link http://www.hexillion.com/samples/#Regex Original pattern found here
     * @param string $address The email address to check
     * @return boolean
     * @static
     * @access public
     */
    public static function ValidateAddress($address) {
        if (function_exists('filter_var')) { //Introduced in PHP 5.2
            if(filter_var($address, FILTER_VALIDATE_EMAIL) === FALSE) {
                return false;
            } else {
                return true;
            }
        } else {
            return preg_match('/^(?:[\w\!\#\$\%\&\'\*\+\-\/\=\?\^\`\{\|\}\~]+\.)*[\w\!\#\$\%\&\'\*\+\-\/\=\?\^\`\{\|\}\~]+@(?:(?:(?:[a-zA-Z0-9_](?:[a-zA-Z0-9_\-](?!\.)){0,61}[a-zA-Z0-9_-]?\.)+[a-zA-Z0-9_](?:[a-zA-Z0-9_\-](?!$)){0,61}[a-zA-Z0-9_]?)|(?:\[(?:(?:[01]?\d{1,2}|2[0-4]\d|25[0-5])\.){3}(?:[01]?\d{1,2}|2[0-4]\d|25[0-5])\]))$/', $address);
        }
    }

    public function MsgHTML($message, $basedir = '') {

        $this->IsHTML(true);
        $this->Body = $message;
        $textMsg = trim(strip_tags(preg_replace('/<(head|title|style|script)[^>]*>.*?<\/\\1>/s','',$message)));
        if (!empty($textMsg) && empty($this->AltBody)) {
            $this->AltBody = html_entity_decode($textMsg);
        }
        if (empty($this->AltBody)) {
            $this->AltBody = 'To view this email message, open it in a program that understands HTML!' . "\n\n";
        }
    }


    /**
     * Sets message type to HTML.
     * @param bool $ishtml
     * @return void
     */
    public function IsHTML($ishtml = true) {
        if ($ishtml) {
            $this->ContentType = 'text/html';
        } else {
            $this->ContentType = 'text/plain';
        }
    }



    public function Send() {


        // CREAR EL PAYLOAD PROCESANDO LOS ELEMENTOS CONFIGURADOS PARA EL ENVÍO

        // * From
        $from = $this->From;
        if($this->FromName) {
            $from = array(
                'email' => $this->From,
                'name' => $this->FromName
            );
        }


        // * Recipients

        $recipients = array();
        $headers_cc = array();
        $header_to = '';

        // se establece al primer to, como el destinatario principal del email (header_to)
        $item = current($this->to);
        $header_to = $item[0];

        // ** Procesar to
        foreach($this->to as $item){
            $address = array(
                'email' => $item[0],
                'header_to' => $header_to
            );
            $address = $item[1] ? $address + array('name' => $item[1]) : $address;

            $recipients[] = array(
                'address' => $address
            );
        }

        // ** Procesar cc
        if(count($this->cc)) {
            foreach ($this->cc as $item) {
                $address = array(
                    'email' => $item[0],
                    'header_to' => $header_to
                );
                $address = $item[1] ? $address + array('name' => $item[1]) : $address;

                $recipients[] = array(
                    'address' => $address
                );
                $headers_cc[] = $item[0];
            }
        }

        // ** Procesar bcc
        if(count($this->bcc)) {
            foreach ($this->bcc as $item) {
                $address = array(
                    'email' => $item[0],
                    'header_to' => $header_to
                );
                $address = $item[1] ? $address + array('name' => $item[1]) : $address;

                $recipients[] = array(
                    'address' => $address
                );
            }
        }

        // ** Procesar reply_to
        $reply_to = '';
        if (count($this->ReplyTo)) {
            $item = current($this->ReplyTo);
            if ($item[1]) {
                $reply_to = $item[1] . ' <' . $item[0] .'>';
            } else {
                $reply_to = $item[0];
            }
        }

        // * Configurar Headers
        $headers = new stdClass();
        if(count($headers_cc)) {
            $headers->CC = implode(',', $headers_cc);
        }


        // * Attachments
        $attachments = array();
        if(count($this->attachment)) {
            foreach($this->attachment as $item) {
                $attachments[] = array(
                    'name' => $item[1],
                    'type' => $item[4],
                    'data' => base64_encode(file_get_contents($item[0]))
                );
            }
        }

        // * Corregir encoding de Subject y Body a utf si es necesario

        $this->Subject = mb_detect_encoding($this->Subject, 'UTF-8', true) === false ? utf8_encode($this->Subject) : $this->Subject;
        $this->Body = mb_detect_encoding($this->Body, 'UTF-8', true) === false ? utf8_encode($this->Body) : $this->Body;
        $this->AltBody = mb_detect_encoding($this->AltBody, 'UTF-8', true) === false ? utf8_encode($this->AltBody) : $this->AltBody;



        // * Crear payload final
        $payload = array(
            'options' => array(
                'click_tracking' => false,
                'inline_css' => true,
                'transactional' => true,
            ),
            'recipients' => $recipients,
            'content' => array(
                'from' => $from,
                'subject' => $this->Subject,
                'text' => $this->AltBody,
                'html' => $this->Body,
                'headers' => $headers,
                'attachments' => $attachments
            )
        );
        if($reply_to){
            $payload['content']['reply_to'] = $reply_to;
        }

        if($this->debug) {
            $debug_payload = $payload;
            if($debug_payload['content']['attachments']) {
                foreach($debug_payload['content']['attachments'] as &$item){
                    $item['data'] = 'hidden';
                }
            }
            $this->debug_text .= "\n" . str_repeat ('-', 80) . "\nPAYLOAD:\n" . str_repeat ('-', 80) . "\n\n" . print_r($debug_payload, 1) . "\n\n";
        }




        // REALIZAR REQUEST

        $headers = array(
            "Content-Type: application/json",
            'Authorization: '. $this->API_KEY
        );

        $ch = curl_init();
        curl_setopt($ch, CURLOPT_URL, $this->API_URL);
        curl_setopt($ch, CURLOPT_RETURNTRANSFER, true);
        curl_setopt($ch, CURLOPT_TIMEOUT, 60);
        curl_setopt($ch, CURLOPT_CUSTOMREQUEST, "POST");
        curl_setopt($ch, CURLOPT_POSTFIELDS, json_encode($payload));
        curl_setopt($ch, CURLOPT_HTTPHEADER, $headers);
        curl_setopt($ch, CURLOPT_FOLLOWLOCATION, 1);
        curl_setopt ($ch, CURLOPT_SSLVERSION, 6); // TLS 1.2

        if($this->debug) {
            $curl_log = tmpfile();
            curl_setopt($ch, CURLOPT_VERBOSE, 1);
            curl_setopt($ch, CURLOPT_STDERR, $curl_log);
        }


        // PROCESAR RESPONSE

        $response = curl_exec($ch);
        $this->result = json_decode($response);

        if($this->debug) {
            rewind($curl_log);
            $output = fread($curl_log, 2048);
            $this->debug_text .= "\n" . str_repeat ('-', 80) . "\nREQUEST VERBOSE:\n" . str_repeat ('-', 80) . "\n\n" . print_r($output, 1) . "\n\n"  ;
            fclose($curl_log);

            $this->debug_text .= "\n" . str_repeat ('-', 80) . "\nRESULT:\n" .  str_repeat ('-', 80) . "\n\n" . print_r($this->result, 1) . "\n\n";
        }

        $res = false;
        if (!isset($this->result)):
            $res = false;
        elseif (isset($this->result->errors)):
            $res = false;
        else:
            $res = true;
        endif;

        curl_close($ch);

        return $res;

    }

}